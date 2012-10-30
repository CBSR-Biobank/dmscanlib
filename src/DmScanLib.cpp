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
#include "dib/Dib.h"
#include "dib/RgbQuad.h"

#include <glog/logging.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

namespace dmscanlib {

const std::string DmScanLib::LIBRARY_NAME("dmscanlib");

bool DmScanLib::loggingInitialized = false;

DmScanLib::DmScanLib(unsigned loggingLevel, bool logToFile) :
		imgScanner(std::move(ImgScanner::create())),
		stdoutOutputEnable(false), textFileOutputEnable(false)
{
	configLogging(loggingLevel, logToFile);
	if (VLOG_IS_ON(2)) {
		util::Time::getTime(starttime);
	}
}

DmScanLib::~DmScanLib() {
	if (VLOG_IS_ON(2)) {
		util::Time::getTime(endtime);
		util::Time::difftime(starttime, endtime, timediff);
		VLOG(2) << "time taken: " << timediff;
	}
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

int DmScanLib::isValidDpi(int dpi) {
	int dpiCap = imgScanner->getScannerCapability();
	return ((dpiCap & CAP_DPI_300) && dpi == 300)
			|| ((dpiCap & CAP_DPI_400) && dpi == 400)
			|| ((dpiCap & CAP_DPI_600) && dpi == 600);
}

void DmScanLib::configLogging(unsigned level, bool useFile) {
	if (loggingInitialized)  return;

	google::InitGoogleLogging(LIBRARY_NAME.c_str());
	FLAGS_v = level;
	FLAGS_stderrthreshold = (level > 0) ? google::INFO : google::ERROR;
	FLAGS_logtostderr = !useFile;
	FLAGS_alsologtostderr = !useFile;

	loggingInitialized = true;
}

void DmScanLib::saveResults(std::string & msg) {
#ifdef DEBUG1
	// FIXME:: is this required for VISUAL STUDIO?
	/*
	 * Could not use C++ streams for Release version of DLL.
	 */
	FILE *fh = fopen("dmscanlib.txt", "w");
	CHECK_NOTNULL(fh);
	fprintf(fh, "%s", msg.c_str());
	fclose(fh);
#else
	std::ofstream myfile;
	myfile.open("dmscanlib.txt");
	myfile << msg;
	myfile.close();

#endif
}

int DmScanLib::scanImage(unsigned dpi, int brightness, int contrast,
		double left, double top, double right, double bottom,
		const char * filename) {
	VLOG(2)
			<< "slScanImage: dpi/" << dpi << " brightness/" << brightness
					<< " contrast/" << contrast << " left/" << left << " top/"
					<< top << " right/" << right << " bottom/" << bottom
					<< " filename/" << filename;

	HANDLE h = imgScanner->acquireImage(dpi, brightness, contrast, left, top,
			right, bottom);
	if (h == NULL) {
		VLOG(2) << "could not acquire image";
		return imgScanner->getErrorCode();
	}
	Dib dib;
	dib.readFromHandle(h);
	if (dib.getDpi() != dpi) {
		return SC_INCORRECT_DPI_SCANNED;
	}
	dib.writeToFile(filename);
	imgScanner->freeImage(h);
	return SC_SUCCESS;
}

int DmScanLib::scanFlatbed(unsigned dpi, int brightness, int contrast,
		const char * filename) {
	VLOG(2)
			<< "slScanFlatbed: dpi/" << dpi << " brightness/" << brightness
					<< " contrast/" << contrast << " filename/" << filename;

	HANDLE h = imgScanner->acquireFlatbed(dpi, brightness, contrast);
	if (h == NULL) {
		VLOG(2) << "could not acquire image";
		return imgScanner->getErrorCode();
	}
	Dib dib;
	dib.readFromHandle(h);
	if (dib.getDpi() != dpi) {
		return SC_INCORRECT_DPI_SCANNED;
	}
	dib.writeToFile(filename);
	imgScanner->freeImage(h);
	return SC_SUCCESS;
}

int DmScanLib::scanAndDecode(unsigned dpi, int brightness, int contrast,
		double left, double top, double right, double bottom,
		const DecodeOptions & decodeOptions,
		std::vector<std::unique_ptr<WellRectangle<unsigned>  > > & wellRects) {
	VLOG(2) << "decodePlate: dpi/" << dpi << " brightness/" << brightness
			<< " contrast/" << contrast
			<< " left/" << left << " top/" << top << " right/" << right
			<< " bottom/" << bottom << decodeOptions;

	HANDLE h;
	int result;

	h = imgScanner->acquireImage(dpi, brightness, contrast, left, top, right,
			bottom);
	if (h == NULL) {
		VLOG(2) << "could not acquire image";
		return imgScanner->getErrorCode();
	}

	Dib image;
	image.readFromHandle(h);
	if (image.getDpi() != dpi) {
		return SC_INCORRECT_DPI_SCANNED;
	}

	image.writeToFile("scanned.bmp");
	result = decodeCommon(image, decodeOptions, "decode.bmp", wellRects);

	imgScanner->freeImage(h);
	VLOG(2) << "decodeCommon returned: " << result;
	return result;
}

int DmScanLib::decodeImageWells(const char * filename,
		const DecodeOptions & decodeOptions,
		std::vector<std::unique_ptr<WellRectangle<unsigned>  > > & wellRects) {

	VLOG(2) << "decodeImage: filename/" << filename
			<< " numWellRects/" << wellRects.size()
			<< decodeOptions;

	Dib image;
	bool readResult = image.readFromFile(filename);
	if (!readResult) {
		return SC_INVALID_IMAGE;
	}

	return decodeCommon(image, decodeOptions, "decode.bmp", wellRects);
}

int DmScanLib::decodeCommon(const Dib & image,
		const DecodeOptions & decodeOptions,
		const std::string &decodedDibFilename,
		std::vector<std::unique_ptr<WellRectangle<unsigned>  > > & wellRects) {

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

void DmScanLib::writeDecodedImage(const Dib & image,
		const std::string & decodedDibFilename) {

	CHECK_NOTNULL(decoder.get());

    const std::vector<WellDecoder *> & decodedWells = decoder->getDecodedWells();
    CHECK(decodedWells.size() > 0);

    RgbQuad colorRed(255, 0, 0);
    RgbQuad colorGreen(0, 255, 0);
	Dib decodedDib(image);
	for (unsigned i = 0, n = decodedWells.size(); i < n; ++i) {
		WellDecoder & decodedWell = *decodedWells[i];
		decodedDib.drawRectangle(decodedWell.getDecodedRectangle(), colorRed);
		decodedDib.drawRectangle(decodedWell.getWellRectangle(), colorGreen);
	}
	decodedDib.writeToFile(decodedDibFilename);
}

const std::vector<WellDecoder *> & DmScanLib::getDecodedWells() const {
	CHECK_NOTNULL(decoder.get());
	return decoder->getDecodedWells();
}


} /* namespace */
