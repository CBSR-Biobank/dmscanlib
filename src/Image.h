#ifndef IMAGE_H_
#define IMAGE_H_

/*
 * Image.h
 *
 *  Created on: 2014-01-20
 *      Author: loyola
 */

#include <dmtx.h>

#include <algorithm>
#include <memory>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#if defined (WIN32) && ! defined(__MINGW32__)
#   define NOMINMAX
#   include <Windows.h>
#else
typedef void* HANDLE;
#endif

namespace dmscanlib {

class Image {
public:
    Image(const char * filename);
    Image(HANDLE handle);
    Image(cv::Mat that);
    virtual ~Image();

    const bool isValid() const {
        return valid;
    }

    const char * getFilename() const {
        return filename;
    }

    const cv::Mat getOriginalImage() const {
        return opencvImage;
    }

    const cv::Size size() const {
        return opencvImage.size();
    }

    DmtxImage * dmtxImage() const;

    std::unique_ptr<const Image> crop(unsigned x, unsigned y, unsigned width, unsigned height) const;

    void drawRectangle(const cv::Rect rect, const cv::Scalar & color);

    bool write(const char * filename) const;

private:

    static const double BLUR_KERNEL_DATA[9];
    static const double BLANK_KERNEL_DATA[9];

    static const cv::Mat BLUR_KERNEL;
    static const cv::Mat BLANK_KERNEL;

    bool valid;
    const char * filename;
    cv::Mat opencvImage;

    void applyFilters();
};

} /* namespace */

#endif /* IMAGE_H_ */
