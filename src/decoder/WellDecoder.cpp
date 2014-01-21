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
        std::unique_ptr<const WellRectangle<unsigned> > _wellRectangle) :
        decoder(_decoder),
                wellRectangle(std::move(_wellRectangle)),
                boundingBox(std::move(wellRectangle->getRectangle().getBoundingBox()))
{
    VLOG(9) << "constructor: bounding box: " << *boundingBox
                      << ", rect: " << wellRectangle->getRectangle();
}

WellDecoder::~WellDecoder() {
}

/*
 * This method runs in its own thread.
 */
void WellDecoder::run() {
    wellImage = decoder.getWorkingImage().crop(
            boundingBox->points[0].x,
            boundingBox->points[0].y,
            boundingBox->points[1].x - boundingBox->points[0].x,
            boundingBox->points[1].y - boundingBox->points[0].y);
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
	VLOG(9) << "getWellRectangle: bbox: " << *wellRectangle->getRectangle().getBoundingBox();

	std::unique_ptr<const BoundingBox<unsigned> >  bbox = wellRectangle->getRectangle().getBoundingBox();

	return cv::Rect(
		bbox->points[0].x,
		bbox->points[0].y,
		bbox->points[1].x - bbox->points[0].x,
		bbox->points[1].y - bbox->points[0].y);
}

const cv::Rect WellDecoder::getDecodedRectangle() const {
	CHECK_NOTNULL(decodedRect.get());
	const BoundingBox<unsigned> & bbox = *decodedRect->getBoundingBox();
	return cv::Rect(
		bbox.points[0].x,
		bbox.points[0].y,
		bbox.points[1].x - bbox.points[0].x,
		bbox.points[1].y - bbox.points[0].y);
}

// the rectangle passed in is in coordinates of the cropped image,
// the rectangle has to be translated into the coordinates of the overall
// image
void WellDecoder::setDecodeRectangle(const Rect<double> & rect, int scale) {
    std::unique_ptr<const Rect<double> > rectCopy;
    if (scale == 1) {
        rectCopy = std::unique_ptr < Rect<double> > (new Rect<double>(rect));
    } else {
        rectCopy = rect.scale(scale);
    }

    Point<unsigned> pt0(static_cast<unsigned>(rectCopy->corners[0].x),
            static_cast<unsigned>(rectCopy->corners[0].y));

    Point<unsigned> pt1(static_cast<unsigned>(rectCopy->corners[1].x),
            static_cast<unsigned>(rectCopy->corners[1].y));

    Point<unsigned> pt2(static_cast<unsigned>(rectCopy->corners[2].x),
            static_cast<unsigned>(rectCopy->corners[2].y));

    Point<unsigned> pt3(static_cast<unsigned>(rectCopy->corners[3].x),
            static_cast<unsigned>(rectCopy->corners[3].y));

    decodedRect = std::unique_ptr<Rect<unsigned>>(
            new Rect<unsigned>(pt0, pt1, pt2, pt3));
    decodedRect = decodedRect->translate(boundingBox->points[0]);
}

std::ostream & operator<<(std::ostream &os, const WellDecoder & m) {
    os << m.getLabel() << ": \"" << m.getMessage() << "\" " << *m.boundingBox;
    return os;
}

} /* namespace */
