#ifndef IMAGE_H_
#define IMAGE_H_

/*
 * Image.h
 *
 *  Created on: 2014-01-20
 *      Author: loyola
 */

#define _CRT_SECURE_NO_DEPRECATE

#include <dmtx.h>

#include <algorithm>
#include <memory>

#if defined (WIN32) && ! defined(__MINGW32__)
#   define NOMINMAX
#   include <Windows.h>
#else
typedef void* HANDLE;
#endif

#include <opencv/cv.h>

namespace dmscanlib {

class Image {
public:
    Image(): valid(false) {}
    Image(const std::string & filename);
    Image(HANDLE handle);
    Image(const Image & that);
    virtual ~Image();

    const bool isValid() const {
        return valid;
    }

    const std::string & getFilename() const {
        return filename;
    }

    const cv::Mat getOriginalImage() const {
        return image;
    }

    const cv::Size size() const {
        return image.size();
    }

    void grayscale(Image & that) const;

    void applyFilters(Image & that) const;

    DmtxImage * dmtxImage() const;

    std::unique_ptr<const Image> crop(unsigned x, unsigned y, unsigned width, unsigned height) const;

    void drawRectangle(const cv::Rect & rect, const cv::Scalar & color);

    void drawLine(const cv::Point & pt1, const cv::Point & pt2, const cv::Scalar & color);

    int write(const std::string & filename) const;


private:
    Image(const cv::Mat & mat);

    cv::Mat image;
    bool valid;
    const std::string filename;
};

} /* namespace */

#endif /* IMAGE_H_ */
