/*
 * Image.cpp
 *
 *  Created on: 2014-01-20
 *      Author: loyola
 */

#include "Image.h"

#include <opencv/highgui.h>

#define GLOG_NO_ABBREVIATED_SEVERITIES
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

Image::Image(const std::string & _filename) : filename(_filename) {
    opencvImage = cvLoadImage(filename.c_str());
    image = cv::Mat(opencvImage);

    valid = (image.data != NULL);

    if (valid) {
        VLOG(1) << "Image::Image: width: " << image.cols
                << ", height: " << image.rows
                << ", depth: " << image.elemSize()
                << ", step: " << image.step1();
    }
}

Image::Image(const Image & that) : filename(""), opencvImage(NULL) {
    if (that.image.data == NULL) {
        throw std::invalid_argument("parameter is null");
    }

    image = that.image;
    valid = true;

    VLOG(5) << "Image::Image: width: " << image.cols
            << ", height: " << image.rows
            << ", depth: " << image.elemSize()
            << ", step: " << image.step1();
}

Image::Image(const cv::Mat & mat) : filename(""), opencvImage(NULL) {
    if (mat.data == NULL) {
        throw std::invalid_argument("parameter is null");
    }

    image = mat;
    valid = true;

    VLOG(5) << "Image::Image: width: " << image.cols
            << ", height: " << image.rows
            << ", depth: " << image.elemSize()
            << ", step: " << image.step1();
}

Image::Image(HANDLE handle) : filename(""), opencvImage(NULL) {
#ifdef WIN32
    BITMAPINFOHEADER *dibHeaderPtr = (BITMAPINFOHEADER *) GlobalLock(handle);

    // if these conditions are not met the Dib cannot be processed
    CHECK(dibHeaderPtr->biSize == 40);
    CHECK(dibHeaderPtr->biPlanes == 1);
    CHECK(dibHeaderPtr->biCompression == 0);
    CHECK(dibHeaderPtr->biXPelsPerMeter == dibHeaderPtr->biYPelsPerMeter);
    CHECK(dibHeaderPtr->biClrImportant == 0);

	unsigned rowBytes = static_cast<unsigned>(
		ceil((dibHeaderPtr->biWidth * dibHeaderPtr->biBitCount) / 32.0)) << 2;
		
	unsigned paletteSize;
    switch (dibHeaderPtr->biBitCount) {
    case 1:
        paletteSize = 2;
		break;
    case 4:
        paletteSize = 16;
		break;
    case 8:
        paletteSize = 256;
		break;
    default:
		paletteSize = 0;
    }

    char * pixels = reinterpret_cast <char *>(dibHeaderPtr)
    + sizeof(BITMAPINFOHEADER) + paletteSize * sizeof(RGBQUAD);

	int channels = 3; // RGB
	IplImage* cv_image = cvCreateImageHeader(
		cvSize(dibHeaderPtr->biWidth, dibHeaderPtr->biHeight), 
		IPL_DEPTH_8U, 
		channels);
	if (!cv_image) {
		throw std::logic_error("invalid image type");
	}
	
	cvSetData(cv_image, pixels, cv_image->widthStep);
	cv::flip(cv::Mat(cv_image), image, 0);

    valid = true;
#else
    valid = false;
#endif
}

Image::~Image() {
    VLOG(1) << "opencvImage: " << opencvImage;
    if (opencvImage) {
        cvReleaseImage(&opencvImage);
    }
    image.release();
}

std::unique_ptr<const Image> Image::grayscale() const {
    cv::Mat grayscale;
    cv::cvtColor(image, grayscale, CV_BGR2GRAY);
    return std::unique_ptr<const Image>(new Image(grayscale));
}

// from: http://opencv-help.blogspot.ca/2013/01/how-to-sharpen-image-using-opencv.html
std::unique_ptr<const Image> Image::applyFilters() const {
    cv::Mat blurredImage;
    cv::Mat enhancedImage;

    cv::GaussianBlur(image, blurredImage, cv::Size(0, 0), 3);
    cv::addWeighted(image, 1.5, blurredImage, -0.5, 0, enhancedImage);
    return std::unique_ptr<const Image>(new Image(enhancedImage));
}


/**
 * Converts an OpenCV image to a DmtxImage.
 *
 * NOTE: The caller must detroy the image.
 */
DmtxImage * Image::dmtxImage() const {
    if (image.elemSize() != 1) {
        throw std::logic_error("invalid bytes per pixel in image");
    }

    DmtxImage * dmtxImage = dmtxImageCreate(
            image.data, image.cols, image.rows, DmtxPack8bppK);
    dmtxImageSetProp(dmtxImage, DmtxPropRowPadBytes, image.step1() - image.cols);
    return dmtxImage;
}

std::unique_ptr<const Image> Image::crop(
        unsigned x,
        unsigned y,
        unsigned width,
        unsigned height) const {
    cv::Rect roi(x, y, width, height);
    cv::Mat croppedImage = image(roi);
    return std::unique_ptr<Image>(new Image(croppedImage));
}

void Image::drawRectangle(const cv::Rect & rect, const cv::Scalar & color) {
    cv::rectangle(image, rect, color);
}

void Image::drawLine(const cv::Point & pt1, const cv::Point & pt2, const cv::Scalar & color) {
    cv::line(image, pt1, pt2, color, 2);
}

int Image::write(const std::string & filename) const {
    VLOG(1) << "write: " << filename;
    IplImage saveImage = image;
    int result = cvSaveImage(filename.c_str(), &saveImage);
    return result;
}

}/* namespace */
