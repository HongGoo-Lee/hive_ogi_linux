#include "ImageProcessor.h"
#include <iostream>

using namespace cv;

void IRDestriper::processFrame(Mat& frame) {
    if (frame.empty() || frame.type() != CV_8UC1) return;
    
    if (!isReady) calibrate(frame);
    else applyFilter(frame);
}

void IRDestriper::calibrate(Mat& frame) {
    Mat colMean;
    reduce(frame, colMean, 0, REDUCE_AVG, CV_32F);
    
    if (accumMean.empty()) accumMean = Mat::zeros(1, frame.cols, CV_32F);
    accumMean += colMean; 
    frameCount++;

    if (frameCount >= CALIBRATION_FRAMES) {
        Mat finalMean = accumMean / CALIBRATION_FRAMES; 
        Scalar overallAvg = mean(finalMean); 
        
        columnNoiseMask = finalMean - overallAvg[0];
        columnNoiseMask.convertTo(columnNoiseMask, CV_8S); 
        isReady = true; 
    }
}

void IRDestriper::applyFilter(Mat& frame) {
    for (int i = 0; i < frame.rows; i++) {
        Mat row = frame.row(i);
        subtract(row, columnNoiseMask, row, noArray(), CV_8U);
    }
}

Mat ImageProcessor::deflicker_fast(Mat currentFrame, int strengthcutoff) {
    if (prevdeflicker.empty() || prevdeflicker.size() != currentFrame.size()) {
        prevdeflicker = currentFrame.clone();
        return currentFrame;
    }
    Mat diff, mask, result = currentFrame.clone();
    
    absdiff(currentFrame, prevdeflicker, diff);
    threshold(diff, mask, strengthcutoff, 255, THRESH_BINARY_INV);
    
    prevdeflicker.copyTo(result, mask);
    prevdeflicker = result.clone(); 
    
    return result;
}

Mat ImageProcessor::preprocess16to8(const Mat& img16) {
    Mat img8;
    normalize(img16, img8, 0, 255, NORM_MINMAX, CV_8U);
    destriper.processFrame(img8);
    return img8;
}

Mat ImageProcessor::processGasDetection(Mat& img8, bool isLeakBtnActive, bool isRecording, const std::string& dateStr, const std::string& timeStr) {
    Mat color_frame;
    cvtColor(img8, color_frame, COLOR_GRAY2BGR);

    medianBlur(img8, filtered5, 5);

    if (img_background.empty() || img_background.size() != img8.size()) {
        img_background = img8.clone();
    }

    subtract(img8, img_background, img_sub);
    threshold(img_sub, img_binary, 7, 255, THRESH_BINARY);

    if (!img_binary2.empty() && !img_binary3.empty()) {
        if (img_binary.size() == img_binary2.size() && img_binary2.size() == img_binary3.size()) {
            subtract(img_binary3, img_binary2, filtered1); 
            subtract(img_binary, img_binary2, filtered2);  
            bitwise_and(filtered1, filtered2, filtered3);  
        }
    }

    img_binary3 = img_binary2.clone(); 
    img_binary2 = img_binary.clone();  
    img_background = img8.clone();     

    if (!filtered3.empty()) {
        for (int i = 0; i < filtered3.rows; i++) {
            uchar* row_ptr = filtered3.ptr<uchar>(i);
            for (int j = 0; j < filtered3.cols; j++) {
                if (row_ptr[j] == 255) {
                    interval_time = 24; 
                    j = filtered3.cols; 
                    i = filtered3.rows;
                }
            }
        }
    }

    if (interval_time > 0 && isLeakBtnActive) {
        for (int k = 0; k < HiveConfig::DISPLAY_HEIGHT; k++) {
            for (int h = 0; h < HiveConfig::DISPLAY_WIDTH; h++) {
                if (img_binary.at<uchar>(k, h) == 255) {
                    color_frame.at<Vec3b>(k, h)[2] = 0;   
                    color_frame.at<Vec3b>(k, h)[1] = 0;   
                    color_frame.at<Vec3b>(k, h)[0] = 255; 
                }
            }
        }
        putText(color_frame, "GAS DETECTED!!", Point(100, 100), 0, 2, Scalar(255, 0, 0), 1, 8);
        interval_time--; 
    }

    if (isRecording) {
        circle(img8, Point(30, 30), 10, Scalar(255), -1);
        putText(img8, "REC " + dateStr + timeStr, Point(50, 35), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255), 2);

        circle(color_frame, Point(30, 30), 10, Scalar(255, 0, 0), -1);
        putText(color_frame, "REC " + dateStr + timeStr, Point(50, 35), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255, 255, 255), 2);
    }

    return color_frame; 
}