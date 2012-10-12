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

WellDecoder::WellDecoder(Decoder & _decoder, WellRectangle<int> & _wellCoordinates)
: decoder(_decoder), wellRectangle(_wellCoordinates) {
}

WellDecoder::~WellDecoder() {
}

/**
 * Invoked in its own thread.
 */
void WellDecoder::run() {
    ostringstream id;
    id << wellRectangle.getLabel();

    getImage();
    decoder.decodeImage(cellImage, id.str(), decodedWell);
    if (!decodedWell.msg.empty()) {
        grid.registerBarcodeMsg(decodedWell.msg);

        RAW_LOG(INFO,
                "run: (%d,%d) - %s", row, col, decodedWell.msg.c_str());
    }
}

std::tr1::shared_ptr<const Dib> WellDecoder::getImage() {
    if (cellImage.get() == NULL) {
        cellImage = grid.getCellImage(*this);
    }
    return cellImage;
}

const string & WellDecoder::getBarcodeMsg() {
    CHECK(!decodedWell.msg.empty());
    return decodedWell.msg;
}

void WellDecoder::writeImage(std::string basename) {
    // do not write diagnostic image is log level is less than 9
    if (!VLOG_IS_ON(5)) return;

    ostringstream fname;
    fname << basename << "-" << row << "-" << col << ".bmp";
    cellImage->writeToFile(fname.str().c_str());
}

void WellDecoder::drawCellBox(Dib & image, const RgbQuad & color) const {
    image.rectangle(parentRect.x, parentRect.y, parentRect.width,
                    parentRect.height, color);
}

void WellDecoder::drawBarcodeBox(Dib & image, const RgbQuad & color) const {
    Point corners[4];

    for (unsigned i = 0; i < 4; ++i) {
        corners[i].x = decodedWell.corners[i].x + parentRect.x;
        corners[i].y = decodedWell.corners[i].y + parentRect.y;
    }

    image.line(corners[0], corners[1], color);
    image.line(corners[1], corners[2], color);
    image.line(corners[2], corners[3], color);
    image.line(corners[3], corners[0], color);
}

ostream & operator<<(ostream &os, WellDecoder & m) {
    os << "\"" << m.decodedWell.msg << "\" (" << m.decodedWell.corners[0].x
       << ", " << m.decodedWell.corners[0].y << "), " << "("
       << m.decodedWell.corners[2].x << ", " << m.decodedWell.corners[2].y
       << "), " << "(" << m.decodedWell.corners[3].x << ", "
       << m.decodedWell.corners[3].y << "), " << "("
       << m.decodedWell.corners[1].x << ", " << m.decodedWell.corners[1].y
       << ")";
    return os;
}
