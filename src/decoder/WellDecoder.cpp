/*
 * DecodedWell.h
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "WellDecoder.h"
#include "dib/Dib.h"
#include "Decoder.h"
#include "geometry.h"

#include <sstream>
#include <glog/logging.h>
#include <glog/raw_logging.h>

#ifdef _VISUALC_
#   include <functional>
#endif

namespace dmscanlib {

WellDecoder::WellDecoder(const Decoder & _decoder,
		std::unique_ptr<const WellRectangle<unsigned> > _wellRectangle) :
		decoder(_decoder), wellRectangle(std::move(_wellRectangle)),
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
    wellImage = decoder.getWorkingImage().crop(*boundingBox);
    decoder.decodeWellRect(*wellImage, *this);
    if (!message.empty()) {
    	//VLOG(3) << "run: " << *this;
    } else {
    	//VLOG(3) << "run: "<< wellRectangle->getLabel() << " - could not be decoded";
    }
}

void WellDecoder::setMessage(const char * message, int messageLength) {
   this->message.assign(message, messageLength);
}

const Rect<unsigned> & WellDecoder::getDecodedRectangle() const {
	CHECK_NOTNULL(decodedRect.get());
	return *decodedRect;
}

// the rectangle passed in is in coordinates of the cropped image,
// the rectangle has to be translated into the coordinates of the overall
// image
void WellDecoder::setDecodeRectangle(const Rect<double> & rect, int scale) {
	std::unique_ptr<const Rect<double> > rectCopy;
	if (scale == 1) {
		rectCopy= std::unique_ptr<Rect<double> >(new Rect<double>(rect));
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

	decodedRect = std::unique_ptr<Rect<unsigned> >(new Rect<unsigned>(
		pt0, pt1, pt2, pt3));
	decodedRect = decodedRect->translate(boundingBox->points[0]);
}

std::ostream & operator<<(std::ostream &os, const WellDecoder & m) {
    os << m.getLabel() << ": \"" << m.getMessage() << "\" "<< *m.boundingBox;
    return os;
}


} /* namespace */
