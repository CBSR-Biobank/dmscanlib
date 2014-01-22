/*
 * DecodedWell.h
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "WellDecoder.h"
#include "Image.h"
#include "Decoder.h"
#include "geometry.h"

#include <sstream>
#include <glog/logging.h>
#include <glog/raw_logging.h>

#ifdef _VISUALC_
#   include <functional>
#endif

namespace dmscanlib {

WellDecoder::WellDecoder(
        const Decoder & _decoder,
        std::unique_ptr<const WellRectangle> _wellRectangle) :
        decoder(_decoder),
        wellRectangle(std::move(_wellRectangle)),
        boundingBox(wellRectangle->getRectangle()),
        decodedRect()
{
    decodedRect.reserve(4);
    VLOG(9) << "constructor: bounding box: " << boundingBox
            << ", rect: " << wellRectangle->getRectangle();
}

WellDecoder::~WellDecoder() {
}

/*
 * This method runs in its own thread.
 */
void WellDecoder::run() {
    wellImage = decoder.getWorkingImage().crop(
            boundingBox.x,
            boundingBox.y,
            boundingBox.width,
            boundingBox.height);
    decoder.decodeWellRect(*wellImage, *this);
    if (!message.empty()) {
        VLOG(3) << "run: " << *this;
    } else {
        VLOG(3) << "run: " << wellRectangle->getLabel() << " - could not be decoded";
    }
}

void WellDecoder::setMessage(const char * message, int messageLength) {
    this->message.assign(message, messageLength);
}


const cv::Rect WellDecoder::getWellRectangle() const {	
    CHECK_NOTNULL(wellRectangle.get());
	VLOG(9) << "getWellRectangle: bbox: " << wellRectangle->getRectangle();

	return wellRectangle->getRectangle();
}

const std::vector<cv::Point> WellDecoder::getDecodedRectangle() const {
	return decodedRect;
}

// the rectangle passed in is in coordinates of the cropped image,
// the rectangle has to be translated into the coordinates of the overall
// image
void WellDecoder::setDecodeRectangle(const Rect<float> & rect, int scale) {
    std::unique_ptr<const Rect<float> > rectCopy;
    if (scale == 1) {
        rectCopy = std::unique_ptr < Rect<float> > (new Rect<float>(rect));
    } else {
        rectCopy = rect.scale(scale);
    }

    cv::Point_<unsigned> pt0(static_cast<unsigned>(rectCopy->corners[0].x),
            static_cast<unsigned>(rectCopy->corners[0].y));

    cv::Point_<unsigned> pt1(static_cast<unsigned>(rectCopy->corners[1].x),
            static_cast<unsigned>(rectCopy->corners[1].y));

    cv::Point_<unsigned> pt2(static_cast<unsigned>(rectCopy->corners[2].x),
            static_cast<unsigned>(rectCopy->corners[2].y));

    cv::Point_<unsigned> pt3(static_cast<unsigned>(rectCopy->corners[3].x),
            static_cast<unsigned>(rectCopy->corners[3].y));

    decodedRect = std::unique_ptr<Rect<unsigned>>(
            new Rect<unsigned>(pt0, pt1, pt2, pt3));
    decodedRect = decodedRect + boundingBox.tl();
}

std::ostream & operator<<(std::ostream &os, const WellDecoder & m) {
    os << m.getLabel() << ": \"" << m.getMessage() << "\" " << *m.boundingBox;
    return os;
}

} /* namespace */
