/*
 * DecodedWell.h
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "WellDecoder.h"
#include "DmScanLibInternal.h"
#include "Dib.h"
#include "Decoder.h"
#include "DecodedWell.h"

#include <glog/logging.h>
#include <glog/raw_logging.h>

#ifdef _VISUALC_
#   include <functional>
#else
#   include <tr1/functional>
#endif

WellDecoder::WellDecoder(const Dib & _image, Decoder & _decoder,
		WellRectangle<unsigned> & _wellRectangle) :
		scannedImage(_image), decoder(_decoder), decodedWell(_wellRectangle) {
}

WellDecoder::~WellDecoder() {
}

/**
 * Invoked in its own thread.
 */
void WellDecoder::run() {
    ostringstream id;
    id << decodedWell.getWellRectangle().getLabel();

    // TODO: get bounding box for well rectangle
    wellImage = scannedImage.crop(0,0,0,0);
    decoder.decodeImage(*wellImage.get(), decodedWell);
    if (!decodedWell.getMessage().empty()) {

        RAW_LOG(INFO, "run: (%s) - %s",
        		decodedWell.getWellRectangle().getLabel().c_str(),
                decodedWell.getMessage().c_str());
    }
}

const string & WellDecoder::getBarcodeMsg() {
    CHECK(!decodedWell.getMessage().empty());
    return decodedWell.getMessage();
}

void WellDecoder::writeImage(std::string basename) {
    // do not write diagnostic image if log level is less than 5
    if (!VLOG_IS_ON(5)) return;

    ostringstream fname;
    fname << basename << "-" << decodedWell.getWellRectangle().getLabel().c_str() << ".bmp";
    wellImage->writeToFile(fname.str().c_str());
}

void WellDecoder::drawCellBox(Dib & image, const RgbQuad & color) const {
    image.drawRectangle(decodedWell.getWellRectangle().getRectangle(), color);
}

void WellDecoder::drawBarcodeBox(Dib & image, const RgbQuad & color) const {
    image.drawRectangle(decodedWell.getDecodedRect(), color);
}

ostream & operator<<(ostream &os, WellDecoder & m) {
	const Rect<unsigned> & rect = m.getDecodedRectangle();

    os << m.getLabel() << ": "
       << "\"" << m.getMessage() << "\" ("
       << rect.corners[0].x << ", " << rect.corners[0].y << "), "
       << "(" << rect.corners[2].x << ", " << rect.corners[2].y << "), "
       << "(" << rect.corners[3].x << ", "  << rect.corners[3].y << "), "
       << "(" << rect.corners[1].x << ", " << rect.corners[1].y  << ")";
    return os;
}
