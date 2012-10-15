/*
 * DecodedWell.h
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "WellDecoder.h"
#include "dib/Dib.h"
#include "Decoder.h"

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

    // TODO: get bounding box for well rectangle
    wellImage = std::move(decoder.getWorkingImage().crop(0,0,0,0));
    decoder.decodeWellRect(*wellImage, *this);
    if (!message.empty()) {
    	RAW_LOG(INFO, "run: (%s) - %s", wellRectangle->getLabel().c_str(),
    			message.c_str());
    }
}

void WellDecoder::setMessage(const char * message, int messageLength) {
   this->message.assign(message, messageLength);
}

const string & WellDecoder::getBarcodeMsg() {
    CHECK(!message.empty());
    return message;
}

void WellDecoder::writeImage(std::string basename) {
    // do not write diagnostic image if log level is less than 5
    if (!VLOG_IS_ON(5)) return;

    ostringstream fname;
    fname << basename << "-" << wellRectangle->getLabel().c_str() << ".bmp";
    wellImage->writeToFile(fname.str().c_str());
}

void WellDecoder::drawCellBox(Dib & image, const RgbQuad & color) const {
    image.drawRectangle(wellRectangle->getRectangle(), color);
}

void WellDecoder::drawBarcodeBox(Dib & image, const RgbQuad & color) const {
    image.drawRectangle(decodedRect, color);
}

void WellDecoder::setCorner(unsigned cornerId, unsigned x, unsigned y) {
     CHECK(cornerId < 4);
     decodedRect.corners[cornerId].x = x;
     decodedRect.corners[cornerId].y = y;
}

ostream & operator<<(ostream &os, WellDecoder & m) {
    os << m.getLabel() << ": \"" << m.getMessage() << "\" "<< m.decodedRect;
    return os;
}
