#ifndef IMAGE_TEXTURE_H
#define IMAGE_TEXTURE_H

#include <opencv2/opencv.hpp>
#include <GLFW/glfw3.h>

class ImageTexture {
private:
    GLuint textureID = 0;
    int width = 0;
    int height = 0;

public:
    ImageTexture() {}
    ~ImageTexture() { 
        if (textureID) glDeleteTextures(1, &textureID); 
    }

    void Update(const cv::Mat& frame) {
        if (frame.empty()) return;

        if (!textureID) {
            glGenTextures(1, &textureID);
        }

        // BGR(OpenCV 기본)을 RGB로 변환
        cv::Mat rgb;
        cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rgb.cols, rgb.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data);

        width = rgb.cols;
        height = rgb.rows;
    }

    GLuint GetID() const { return textureID; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
};

#endif // IMAGE_TEXTURE_H