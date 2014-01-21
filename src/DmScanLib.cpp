/*******************************************************************************
 * Canadian Biosample Repository
 *
 * DmScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 * Dmscanlib is a software library and standalone application that scans
 * and decodes libdmtx compatible test-tubes. It is currently designed
 * to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 *  Copyright (C) 2010 Canadian Biosample Repository
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#ifdef _VISUALC_
// disable warnings about fopen
#pragma warning(disable : 4996)
#endif

#include "DmScanLib.h"
#include "imgscanner/ImgScanner.h"
#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellDecoder.h"
#include "Image.h"

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

namespace dmscanlib {

const std::string DmScanLib::LIBRARY_NAME("dmscanlib");

bool DmScanLib::loggingInitialized = false;

DmScanLib::DmScanLib() :
        imgScanner(std::move(ImgScanner::create()))
{
}

DmScanLib::DmScanLib(unsigned loggingLevel, bool logToFile) :
        imgScanner(std::move(ImgScanner::create()))
{
    configLogging(loggingLevel, logToFile);
}

DmScanLib::~DmScanLib() {
}

int DmScanLib::isTwainAvailable() {
    if (imgScanner->twainAvailable()) {
        return SC_SUCCESS;
    }
    return SC_TWAIN_UNAVAIL;
}

int DmScanLib::selectSourceAsDefault() {
    if (imgScanner->selectSourceAsDefault()) {
        return SC_SUCCESS;
    }
    return SC_FAIL;
}

int DmScanLib::getScannerCapability() {
    return imgScanner->getScannerCapability();
}

void DmScanLib::configLogging(unsigned level, bool useFile) {
    if (loggingInitialized)
        return;

    google::InitGoogleLogging(LIBRARY_NAME.c_str());
    FLAGS_v = level;

#ifdef _VISUALC_
    FLAGS_stderrthreshold = (level > 0) ? google::GLOG_INFO : google::GLOG_ERROR;
#else
    FLAGS_stderrthreshold = (level > 0) ? google::INFO : google::ERROR;
#endif

    FLAGS_logtostderr = !useFile;
    FLAGS_alsologtostderr = false;

    loggingInitialized = true;
}

int DmScanLib::scanImage(unsigned dpi, int brightness, int contrast,
        const ScanRegion<double> & bbox, const char * filename) {
    if (filename == NULL) {
        throw std::invalid_argument("filename is null");
    }

    VLOG(1) << "scanImage: dpi/" << dpi << " brightness/" << brightness
                      << " contrast/" << contrast << bbox
                      << " filename/" << filename;

    HANDLE h = imgScanner->acquireImage(dpi, brightness, contrast, bbox);
    if (h == NULL) {
        VLOG(1) << "could not acquire image";
        return imgScanner->getErrorCode();
    }
    Image image(h);
    image.write(filename);
    imgScanner->freeImage(h);
    return SC_SUCCESS;
}

int DmScanLib::scanFlatbed(unsigned dpi, int brightness, int contrast,
        const char * filename) {
    if (filename == NULL) {
        throw std::invalid_argument("filename is null");
    }

    VLOG(1) << "scanFlatbed: dpi/" << dpi << " brightness/" << brightness
                      << " contrast/" << contrast << " filename/" << filename;

    HANDLE h = imgScanner->acquireFlatbed(dpi, brightness, contrast);
    if (h == NULL) {
        VLOG(1) << "could not acquire image";
        return imgScanner->getErrorCode();
    }
    Image image(h);
    image.write(filename);
    imgScanner->freeImage(h);
    return SC_SUCCESS;
}

int DmScanLib::scanAndDecode(unsigned dpi, int brightness, int contrast,
        const ScanRegion<double> & region, const DecodeOptions & decodeOptions,
        std::vector<std::unique_ptr<const WellRectangle<double> > > & wellRects) {
    VLOG(3) << "scanAndDecode: dpi/" << dpi << " brightness/" << brightness
                      << " contrast/" << contrast
                      << " " << region << " " << decodeOptions;

    HANDLE h;
    int result;

    h = imgScanner->acquireImage(dpi, brightness, contrast, region);
    if (h == NULL) {
        VLOG(1) << "could not acquire image";
        return imgScanner->getErrorCode();
    }

    Image image(h);
    image.write("scanned.bmp");
    result = decodeCommon(image, decodeOptions, "decode.bmp", wellRects);

    imgScanner->freeImage(h);
    VLOG(1) << "decodeCommon returned: " << result;
    return result;
}

int DmScanLib::decodeImageWells(
        const char * filename,
        const DecodeOptions & decodeOptions,
        std::vector<std::unique_ptr<const WellRectangle<double> > > & wellRects) {

    VLOG(1) << "decodeImageWells: filename/" << filename
            << " numWellRects/" << wellRects.size()
            << " " << decodeOptions;

    Image image(filename);
    if (!image.isValid()) {
        return SC_INVALID_IMAGE;
    }

    return decodeCommon(image, decodeOptions, "decode.bmp", wellRects);
}

int DmScanLib::decodeCommon(const Image & image,
        const DecodeOptions & decodeOptions,
        const std::string &decodedDibFilename,
        std::vector<std::unique_ptr<const WellRectangle<double> > > & wellRects) {

    decoder = std::unique_ptr<Decoder>(new Decoder(image, decodeOptions, wellRects));
    int result = decoder->decodeWellRects();

    if (result != SC_SUCCESS) {
        return result;
    }

    const unsigned decodedWellCount = decoder->getDecodedWellCount();

    if (decodedWellCount == 0) {
        return SC_INVALID_NOTHING_DECODED;
    }

    writeDecodedImage(image, decodedDibFilename);

    return SC_SUCCESS;
}

void DmScanLib::writeDecodedImage(
        const Image & image,
        const std::string & decodedDibFilename) {

    CHECK_NOTNULL(decoder.get());

    std::vector<std::unique_ptr<WellDecoder> > & wellDecoders = decoder->getWellDecoders();
    CHECK(wellDecoders.size() > 0);

    const std::map<std::string, const WellDecoder *> & decodedWells = decoder->getDecodedWells();
    CHECK(decodedWells.size() > 0);

    cv::Scalar colorBlue(0, 0, 255);
    cv::Scalar colorRed(255, 0, 0);
    cv::Scalar colorGreen(0, 255, 0);

    Image decodedImage(image);

    for (unsigned i = 0, n = wellDecoders.size(); i < n; ++i) {
        decodedImage.drawRectangle(wellDecoders[i]->getWellRectangle(), colorBlue);
    }

    for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells.begin();
            ii != decodedWells.end(); ++ii) {
        const WellDecoder & decodedWell = *(ii->second);
        const BoundingBox<unsigned> & bboxDecoded = *decodedWell.getDecodedRectangle().getBoundingBox();

        const cv::Rect r(
                bboxDecoded.points[0].x,
                bboxDecoded.points[0].y,
                bboxDecoded.points[1].x - bboxDecoded.points[0].x,
                bboxDecoded.points[1].y - bboxDecoded.points[0].y);
        decodedImage.drawRectangle(r, colorRed);

        const cv::Rect & wellBbox = decodedWell.getWellRectangle();
        decodedImage.drawRectangle(wellBbox, colorGreen);
    }
    decodedImage.write(decodedDibFilename.c_str());
}

const unsigned DmScanLib::getDecodedWellCount() {
    if (decoder == NULL) {
        throw std::logic_error("decoder is null");
    }
    return decoder->getDecodedWellCount();
}

const std::map<std::string, const WellDecoder *> & DmScanLib::getDecodedWells() const {
    CHECK_NOTNULL(decoder.get());
    return decoder->getDecodedWells();
}

} /* namespace */
