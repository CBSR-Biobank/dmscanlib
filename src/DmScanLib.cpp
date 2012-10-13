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
#include "DmScanLibInternal.h"
#include "ImgScanner.h"
#include "ImgScanner.h"
#include "Decoder.h"
#include "DecodeOptions.h"
#include "Dib.h"

#include <glog/logging.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

using namespace std;

const string DmScanLib::LIBRARY_NAME("dmscanlib");

bool DmScanLib::loggingInitialized = false;

DmScanLib::DmScanLib(unsigned loggingLevel, bool logToFile) :
		imgScanner(ImgScanner::create()), stdoutOutputEnable(false), textFileOutputEnable(
				false) {

	configLogging(loggingLevel, logToFile);
	if (VLOG_IS_ON(2)) {
		Util::getTime(starttime);
	}
}

DmScanLib::~DmScanLib() {
	if (VLOG_IS_ON(2)) {
		Util::getTime(endtime);
		Util::difftime(starttime, endtime, timediff);
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
	if (!loggingInitialized) {
		google::InitGoogleLogging(LIBRARY_NAME.c_str());
		FLAGS_v = level;
		FLAGS_stderrthreshold = (level > 0) ? google::INFO : google::ERROR;
		FLAGS_logtostderr = !useFile;
		FLAGS_alsologtostderr = !useFile;

		loggingInitialized = true;
	}
}

void DmScanLib::saveResults(string & msg) {
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
	ofstream myfile;
	myfile.open("dmscanlib.txt");
	myfile << msg;
	myfile.close();

#endif
}

int DmScanLib::scanImage(unsigned dpi, int brightness, int contrast,
		double left, double top, double right, double bottom,
		const string &filename) {
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
		const string & filename) {
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
        double scanGap, unsigned squareDev, unsigned edgeThresh,
        unsigned corrections, double cellDistance) {
	VLOG(2)
			<< "decodePlate: dpi/" << dpi << " brightness/" << brightness
					<< " contrast/" << contrast
					<< " left/" << left << " top/" << top << " right/" << right
					<< " bottom/" << bottom << " scanGap/" << scanGap
					<< " squareDev/" << squareDev << " edgeThresh/"
					<< edgeThresh << " corrections/" << corrections
					<< " cellDistance/" << cellDistance;

	HANDLE h;
	int result;

	this->scanGap = scanGap;
	this->squareDev = squareDev;
	this->edgeThresh = edgeThresh;
	this->corrections = corrections;
	this->cellDistance = cellDistance;

	h = imgScanner->acquireImage(dpi, brightness, contrast, left, top, right,
			bottom);
	if (h == NULL) {
		VLOG(2) << "could not acquire image";
		return imgScanner->getErrorCode();
	}

	image = std::tr1::shared_ptr<Dib>(new Dib());
	image->readFromHandle(h);
	if (image->getDpi() != dpi) {
		return SC_INCORRECT_DPI_SCANNED;
	}

	image->writeToFile("scanned.bmp");
	result = decodeCommon("decode.bmp");

	imgScanner->freeImage(h);
	VLOG(2) << "decodeCommon returned: " << result;
	return result;
}

int DmScanLib::decodeImage(const char * filename, DecodeOptions & decodeOptions,
		vector<std::tr1::shared_ptr<WellRectangle<double>  > > & wellRects) {

	VLOG(2)
			<< "decodeImage: filename/" << filename
					<< " scanGap/" << scanGap << " squareDev/" << squareDev
					<< " edgeThresh/" << edgeThresh << " corrections/"
					<< corrections << " cellDistance/" << cellDistance;

	this->scanGap = scanGap;
	this->squareDev = squareDev;
	this->edgeThresh = edgeThresh;
	this->corrections = corrections;
	this->cellDistance = cellDistance;

	image = std::tr1::shared_ptr<Dib>(new Dib());
	bool readResult = image->readFromFile(filename);
	if (!readResult) {
		return SC_INVALID_IMAGE;
	}

	int result = decodeCommon("decode.bmp");
	return result;
}

int DmScanLib::decodeCommon(const string &markedDibFilename) {
	const unsigned dpi = image->getDpi();

	VLOG(2) << "DecodeCommon: dpi/" << dpi;

	if ((dpi != 300) && (dpi != 400) && (dpi != 600)) {
		return SC_INVALID_DPI;
	}

	decoder = std::tr1::shared_ptr<Decoder>(
			new Decoder(dpi, scanGap, squareDev, edgeThresh, corrections,
					cellDistance));

//	applyFilters();
//
//	if (decodeCount == 0) {
//		return SC_INVALID_IMAGE;
//	}
//
//	// only get here if some cells were decoded
//	if (stdoutOutputEnable || textFileOutputEnable) {
//		string msg;
//		palletGrid->formatCellMessages(msg);
//
//		if (textFileOutputEnable) {
//			saveResults(msg);
//		}
//
//		if (stdoutOutputEnable) {
//			cout << msg;
//		}
//	}
	return SC_SUCCESS;
}

void DmScanLib::applyFilters() {
	if (image->getBitsPerPixel() != 8) {
		image->convertGrayscale();
	}
	image->tpPresetFilter();
	if (VLOG_IS_ON(2)) {
		image->writeToFile("filtered.bmp");
	}
}

//std::vector<std::tr1::shared_ptr<WellDecoder> > & DmScanLib::getDecodedCells() {
	//return palletGrid->getDecodedCells();
//}

int slIsTwainAvailable() {
	DmScanLib dmScanLib(0);
	return dmScanLib.isTwainAvailable();
}

int slSelectSourceAsDefault() {
	DmScanLib dmScanLib(0);
	return dmScanLib.selectSourceAsDefault();
}

int slGetScannerCapability() {
	DmScanLib dmScanLib(0);
	return dmScanLib.getScannerCapability();
}

int slScanFlatbed(unsigned verbose, unsigned dpi, int brightness, int contrast,
		const char * filename) {
	DmScanLib dmScanLib(verbose);
	return dmScanLib.scanFlatbed(dpi, brightness, contrast, filename);
}

int slScanImage(unsigned verbose, unsigned dpi, int brightness, int contrast,
		double left, double top, double right, double bottom,
		const char * filename) {
	DmScanLib dmScanLib(verbose);
	return dmScanLib.scanImage(dpi, brightness, contrast, left, top, right,
			bottom, filename);
}

int slScanAndDecode(unsigned verbose, unsigned dpi, int brightness, int contrast,
		unsigned plateNum, double left, double top, double right, double bottom,
		double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance, double gapX, double gapY,
		unsigned profileA, unsigned profileB, unsigned profileC,
		unsigned orientation) {
	DmScanLib dmScanLib(verbose);
	dmScanLib.setTextFileOutputEnable(true);
	return dmScanLib.scanAndDecode(dpi, brightness, contrast, left, top,
			right, bottom, scanGap, squareDev, edgeThresh, corrections,
			cellDistance);
}

int slDecodeImage(unsigned verbose, unsigned plateNum, const char * filename,
		double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance, double gapX, double gapY,
		unsigned profileA, unsigned profileB, unsigned profileC,
		unsigned orientation) {
	DmScanLib dmScanLib(verbose);
	dmScanLib.setTextFileOutputEnable(true);
//	return dmScanLib.decodeImage(plateNum, filename, scanGap, squareDev,
//			edgeThresh, corrections, cellDistance, gapX, gapY, profileA,
//			profileB, profileC, orientation);
	return -1;
}

