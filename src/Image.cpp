/*
 * Image.cpp
 *
 *  Created on: 2014-01-20
 *      Author: loyola
 */

#include "Image.h"

#include <opencv/highgui.h>
#include <glog/logging.h>

namespace dmscanlib {

// expects blurred image
const double Image::BLUR_KERNEL_DATA[9] = {
        0.0, 0.2, 0.0, 0.2, 0.2, 0.2, 0.0, 0.2, 0.0
};

// expects sharp image
const double Image::BLANK_KERNEL_DATA[9] = {
        0.06185567, 0.12371134, 0.06185567,
        0.12371134, 0.257731959, 0.12371134,
        0.06185567, 0.12371134, 0.06185567
};

const cv::Mat Image::BLUR_KERNEL(3, 3, CV_64F, (void *) &Image::BLUR_KERNEL_DATA[0]);

const cv::Mat Image::BLANK_KERNEL(3, 3, CV_64F, (void *) &Image::BLANK_KERNEL_DATA);

Image::Image(const char * _filename) : filename(_filename) {
    opencvImage = cv::imread(filename);

    valid = (opencvImage.data != NULL);

    if (valid) {
        VLOG(5) << "Image::Image: width: " << opencvImage.cols
                << ", height: " << opencvImage.rows
                << ", depth: " << opencvImage.elemSize()
                << ", step: " << opencvImage.step1();
    }
}

Image::Image(cv::Mat that) {
    if (that.data == NULL) {
        throw std::invalid_argument("parameter is null");
    }

    opencvImage = that;
    valid = true;

    VLOG(5) << "Image::Image: width: " << opencvImage.cols
            << ", height: " << opencvImage.rows
            << ", depth: " << opencvImage.elemSize()
            << ", step: " << opencvImage.step1();
}

Image::Image(HANDLE handle) {
#ifdef WIN32
    BITMAPINFOHEADER *dibHeaderPtr = (BITMAPINFOHEADER *) GlobalLock(handle);

    // if these conditions are not met the Dib cannot be processed
    CHECK(dibHeaderPtr->biSize == 40);
    CHECK(dibHeaderPtr->biPlanes == 1);
    CHECK(dibHeaderPtr->biCompression == 0);
    CHECK(dibHeaderPtr->biXPelsPerMeter == dibHeaderPtr->biYPelsPerMeter);
    CHECK(dibHeaderPtr->biClrImportant == 0);

    init(dibHeaderPtr->biWidth, dibHeaderPtr->biHeight,
            dibHeaderPtr->biBitCount, dibHeaderPtr->biXPelsPerMeter, false);

    pixels = reinterpret_cast <unsigned char *>(dibHeaderPtr)
    + sizeof(BITMAPINFOHEADER) + paletteSize * sizeof(RgbQuad);

    VLOG(3) << "readFromHandle: "
    << " size/" << size
    << " width/" << width
    << " height/" << height
    << " colorBits/" << colorBits
    << " imageSize/" << imageSize
    << " rowBytes/" << rowBytes
    << " paddingBytes/" << rowPaddingBytes << " dpi/" << getDpi();

    valid = true;
#else
    valid = false;
#endif
}

Image::~Image() {
}

std::unique_ptr<const Image> Image::grayscale() const {
    cv::Mat greyscale(opencvImage.size(), opencvImage.type());
    cv::cvtColor(opencvImage, greyscale, CV_BGR2GRAY);
    return std::unique_ptr<const Image>(new Image(greyscale));
}

std::unique_ptr<const Image> Image::applyFilters() const {
    //cv::Mat blurredImage;
    cv::Mat enhancedImage;
    cv::Point anchor(-1, -1);
    double delta = 0;
    int ddepth = -1;

    //cv::filter2D(opencvImage, blurredImage, ddepth , Image::BLUR_KERNEL, anchor, delta);
    cv::filter2D(opencvImage, enhancedImage, ddepth , Image::BLANK_KERNEL, anchor, delta);
    return std::unique_ptr<const Image>(new Image(enhancedImage));
}


/**
 * Converts an OpenCV image to a DmtxImage.
 *
 * NOTE: The caller must detroy the image.
 */
DmtxImage * Image::dmtxImage() const {
    if (opencvImage.elemSize() != 1) {
        throw std::logic_error("invalid bytes per pixel in image");
    }

    DmtxImage * dmtxImage = dmtxImageCreate(
            opencvImage.data, opencvImage.cols, opencvImage.rows, DmtxPack8bppK);
    dmtxImageSetProp(dmtxImage, DmtxPropRowPadBytes, opencvImage.step1() - opencvImage.cols);
    return dmtxImage;
}

std::unique_ptr<const Image> Image::crop(
        unsigned x,
        unsigned y,
        unsigned width,
        unsigned height) const {
    cv::Rect roi(x, y, width, height);
    cv::Mat croppedImage = opencvImage(roi);
    return std::unique_ptr<Image>(new Image(croppedImage));
}

void Image::drawRectangle(const cv::Rect rect, const cv::Scalar & color) {
    cv::rectangle(opencvImage, rect, color);

}

bool Image::write(const char * filename) const {
    return cv::imwrite(filename, opencvImage);
}

}/* namespace */
