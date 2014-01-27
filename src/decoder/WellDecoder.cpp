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

#include <sstream>

#define GLOG_NO_ABBREVIATED_SEVERITIES
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
        rectangle(wellRectangle->getRectangle()),
        decodedQuad()
{
    decodedQuad.reserve(4);
    VLOG(9) << "constructor: bounding box: " << rectangle
            << ", rect: " << wellRectangle->getRectangle();
}

WellDecoder::~WellDecoder() {
}

/*
 * This method runs in its own thread.
 */
void WellDecoder::run() {
    wellImage = decoder.getWorkingImage().crop(
            rectangle.x,
            rectangle.y,
            rectangle.width,
            rectangle.height);
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

// the quadrilateral passed in is in coordinates of the cropped image,
// the quadrilateral has to be translated into the coordinates of the overall
// image
void WellDecoder::setDecodeQuad(const cv::Point2f (&points)[4]) {
    const cv::Point & bboxTl = rectangle.tl();
    for (unsigned i = 0; i < 4; ++i) {
        const cv::Point pt = points[i];
        decodedQuad.push_back(pt + bboxTl);
    }
}

std::ostream & operator<<(std::ostream &os, const WellDecoder & m) {
    os << m.getLabel() << ": \"" << m.getMessage() << "\" " << m.rectangle;
    return os;
}

} /* namespace */
