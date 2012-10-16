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
#else
//#   include <tr1/functional>
#endif

WellDecoder::WellDecoder(const Decoder & _decoder,
		unique_ptr<const WellRectangle<unsigned>> _wellRectangle) :
		decoder(_decoder), wellRectangle(std::move(_wellRectangle)),
		boundingBox(wellRectangle->getRectangle()),
		decodedRect(0, 0, 0, 0, 0, 0, 0, 0)
{
}

WellDecoder::~WellDecoder() {
}

/**
 * Invoked in its own thread.
 */
void WellDecoder::run() {
    ostringstream id;
    id << wellRectangle->getLabel();

    wellImage = std::move(decoder.getWorkingImage().crop(boundingBox));
    decoder.decodeWellRect(*wellImage, *this);
    if (!message.empty()) {
    	RAW_LOG(INFO, "run: (%s) - %s", wellRectangle->getLabel().c_str(),
    			message.c_str());
    }

    // translate decodedRect to image coordinates
}

void WellDecoder::setMessage(const char * message, int messageLength) {
   this->message.assign(message, messageLength);
}

// the rectangle passed in is in coordinates of the cropped image,
// the rectangle has to be translated into the coordinates of the overall
// image
void WellDecoder::setDecodeRectangle(Rect<unsigned> & rect) {
	decodedRect = rect;
	decodedRect.translate(boundingBox.points[0]);
}

ostream & operator<<(ostream &os, WellDecoder & m) {
    os << m.getLabel() << ": \"" << m.getMessage() << "\" "<< m.decodedRect;
    return os;
}
