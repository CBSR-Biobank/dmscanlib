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
#include "ImgScannerFactory.h"
#include "ImgScanner.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Decoder.h"
#include "Dib.h"
#include "BarcodeInfo.h"

#include <stdio.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

//using namespace std;

bool DmScanLib::loggingInitialized = false;

DmScanLib::DmScanLib() :
		textFileOutputEnable(false) {
	imgScanner = ImgScannerFactory::getImgScanner();
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

int DmScanLib::isValidDpi(int dpi) {
	int dpiCap = imgScanner->getScannerCapability();
	return ((dpiCap & CAP_DPI_300) && dpi == 300)
			|| ((dpiCap & CAP_DPI_400) && dpi == 400)
			|| ((dpiCap & CAP_DPI_600) && dpi == 600);
}

void DmScanLib::configLogging(unsigned level, bool useFile) {
	if (!loggingInitialized) {
		if (useFile) {
			ua::LoggerSinkFile::Instance().setFile("dmscanlib.log");
			ua::LoggerSinkFile::Instance().showHeader(true);
			ua::logstream.sink(ua::LoggerSinkFile::Instance());
		} else {
			ua::LoggerSinkStdout::Instance().showHeader(true);
			ua::logstream.sink(ua::LoggerSinkStdout::Instance());
		}
		ua::Logger::Instance().subSysHeaderSet(1, "DmScanLib");
		loggingInitialized = true;
	}

	ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m, level);
}

vector<BarcodeInfo *> & DmScanLib::getBarcodes() {
	return decoder->getBarcodes();
}

/*
 * Could not use C++ streams for Release version of DLL.
 */
void DmScanLib::saveResults(string & msg) {
	FILE *fh = fopen("dmscanlib.txt", "w");
	UA_ASSERT_NOT_NULL(fh);
	fprintf(fh, "%s", msg.c_str());
	fclose(fh);
}

void DmScanLib::formatCellMessages(unsigned plateNum, string & msg) {
	ostringstream out;
	out << "#Plate,Row,Col,Barcode" << endl;

	vector<BarcodeInfo *> & barcodes = decoder->getBarcodes();
	for (unsigned i = 0, n = barcodes.size(); i < n; ++i) {
		BarcodeInfo & info = *barcodes[i];
		const char * msg = info.getMsg().c_str();
		if (msg == NULL) {
			continue;
		}
		out << plateNum << "," << static_cast<char>('A' + info.getRow()) << ","
				<< (info.getCol() + 1) << "," << msg << endl;
	}
	msg = out.str();
}

int DmScanLib::scanImage(unsigned verbose, unsigned dpi, int brightness,
		int contrast, double left, double top, double right, double bottom,
		const char *filename) {
	configLogging(verbose);
	if (filename == NULL) {
		UA_DOUT(1, 3, "slScanImage: no file name specified");
		return SC_FAIL;
	}

	UA_DOUT(
			1,
			3,
			"slScanImage: dpi/" << dpi << " brightness/" << brightness << " contrast/" << contrast << " left/" << left << " top/" << top << " right/" << right << " bottom/" << bottom << " filename/" << filename);

	HANDLE h = imgScanner->acquireImage(dpi, brightness, contrast, left, top,
			right, bottom);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not acquire image");
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

int DmScanLib::scanFlatbed(unsigned verbose, unsigned dpi, int brightness,
		int contrast, const char *filename) {
	configLogging(verbose);
	if (filename == NULL) {
		UA_DOUT(1, 3, "slScanFlatbed: no file name specified");
		return SC_FAIL;
	}

	UA_DOUT(
			1,
			3,
			"slScanFlatbed: dpi/" << dpi << " brightness/" << brightness << " contrast/" << contrast << " filename/" << filename);

	HANDLE h = imgScanner->acquireFlatbed(dpi, brightness, contrast);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not acquire image");
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

int DmScanLib::decodePlate(unsigned verbose, unsigned dpi, int brightness,
		int contrast, unsigned plateNum, double left, double top, double right,
		double bottom, double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance, double gapX, double gapY,
		unsigned profileA, unsigned profileB, unsigned profileC,
		bool outputBarcodes, unsigned orientation) {
	configLogging(verbose);
	UA_DOUT(
			1,
			3,
			"decodePlate: dpi/" << dpi << " brightness/" << brightness << " contrast/" << contrast << " plateNum/" << plateNum << " left/" << left << " top/" << top << " right/" << right << " bottom/" << bottom << " scanGap/" << scanGap << " squareDev/" << squareDev << " edgeThresh/" << edgeThresh << " corrections/" << corrections << " cellDistance/" << cellDistance << " gapX/" << gapX << " gapY/" << gapY << " orientation/" << orientation);

	if ((plateNum < MIN_PLATE_NUM) || (plateNum > MAX_PLATE_NUM)) {
		return SC_INVALID_PLATE_NUM;
	}

	HANDLE h;
	int result;
	Dib dib;

	Util::getTime(starttime);
	h = imgScanner->acquireImage(dpi, brightness, contrast, left, top, right,
			bottom);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not acquire plate image: " << plateNum);
		return imgScanner->getErrorCode();
	}

	dib.readFromHandle(h);
	if (dib.getDpi() != dpi) {
		return SC_INCORRECT_DPI_SCANNED;
	}

	dib.writeToFile("scanned.bmp");
	result = decodeCommon(plateNum, dib, scanGap, squareDev, edgeThresh,
			corrections, cellDistance, gapX, gapY, profileA, profileB, profileC,
			orientation, outputBarcodes, "decode.bmp");

	imgScanner->freeImage(h);
	UA_DOUT(1, 1, "decodeCommon returned: " << result);
	return result;
}

int DmScanLib::decodeImage(unsigned verbose, unsigned plateNum,
		const char *filename, double scanGap, unsigned squareDev,
		unsigned edgeThresh, unsigned corrections, double cellDistance,
		double gapX, double gapY, unsigned profileA, unsigned profileB,
		unsigned profileC, bool outputBarcodes, unsigned orientation) {
	configLogging(verbose);
	UA_DOUT(
			1,
			3,
			"decodeImage: plateNum/" << plateNum << " filename/" << filename << " scanGap/" << scanGap << " squareDev/" << squareDev << " edgeThresh/" << edgeThresh << " corrections/" << corrections << " cellDistance/" << cellDistance << " gapX/" << gapX << " gapY/" << gapY << " orientation/" << orientation);

	if ((plateNum < MIN_PLATE_NUM) || (plateNum > MAX_PLATE_NUM)) {
		return SC_INVALID_PLATE_NUM;
	}

	if (filename == NULL) {
		return SC_FAIL;
	}

	Util::getTime(starttime);

	Dib dib;
	dib.readFromFile(filename);

	int result = decodeCommon(plateNum, dib, scanGap, squareDev, edgeThresh,
			corrections, cellDistance, gapX, gapY, profileA, profileB, profileC,
			orientation, outputBarcodes, "decode.bmp");
	return result;
}

int DmScanLib::decodeCommon(unsigned plateNum, Dib & dib, double scanGap,
		unsigned squareDev, unsigned edgeThresh, unsigned corrections,
		double cellDistance, double gapX, double gapY, unsigned profileA,
		unsigned profileB, unsigned profileC, unsigned orientation,
		bool outputBarcodes, const char *markedDibFilename) {

	const unsigned profileWords[3] = { profileA, profileB, profileC };
	unsigned dpi = dib.getDpi();
	UA_DOUT(1, 3, "DecodeCommon: dpi/" << dpi);
	if ((dpi != 300) && (dpi != 400) && (dpi != 600)) {
		return SC_INVALID_DPI;
	}

	Decoder::ProcessResult result;

	PalletGrid::Orientation palletOrientation = ((orientation == 0) ?
	PalletGrid::ORIENTATION_HORIZONTAL :
	PalletGrid::ORIENTATION_VERTICAL);

	unsigned gapXpixels = static_cast<unsigned>(dpi * gapX);unsigned
	gapYpixels = static_cast<unsigned>(dpi * gapY);

auto_ptr	<PalletGrid> palletGrid(
			new PalletGrid(palletOrientation, dib.getWidth(), dib.getHeight(),
					gapXpixels, gapYpixels, profileWords));

	if (!palletGrid->isImageValid()) {
		return SC_INVALID_IMAGE;
	}

	decoder = auto_ptr < Decoder
			> (new Decoder(scanGap, squareDev, edgeThresh, corrections,
					cellDistance, outputBarcodes, palletGrid.get()));

	/*--- apply filters ---*/
	auto_ptr<Dib> filteredDib(Dib::convertGrayscale(dib));

	filteredDib->tpPresetFilter();
	UA_DEBUG( filteredDib->writeToFile("filtered.bmp"));

	/*--- obtain barcodes ---*/
	result = decoder->processImageRegions(filteredDib.get());

	decoder->imageShowBarcodes(dib, 0);
	if (result == Decoder::OK)
		dib.writeToFile(markedDibFilename);
	else
		dib.writeToFile("decode.partial.bmp");

	if (result == Decoder::IMG_INVALID) {
		return SC_INVALID_IMAGE;
	}

	// only get here if decoder returned Decoder::OK
	if (textFileOutputEnable) {
		string msg;
		formatCellMessages(plateNum, msg);
		saveResults(msg);
	}

	Util::getTime(endtime);
	Util::difftiime(starttime, endtime, timediff);
	UA_DOUT(1, 1, "decodeCommon: time taken: " << timediff);
	return SC_SUCCESS;
}

int slIsTwainAvailable() {
	DmScanLib dmScanLib;
	return dmScanLib.isTwainAvailable();
}

int slSelectSourceAsDefault() {
	DmScanLib dmScanLib;
	return dmScanLib.selectSourceAsDefault();
}

int slGetScannerCapability() {
	DmScanLib dmScanLib;
	return dmScanLib.getScannerCapability();
}

int slScanFlatbed(unsigned verbose, unsigned dpi, int brightness, int contrast,
		const char * filename) {
	DmScanLib dmScanLib;
	return dmScanLib.scanFlatbed(verbose, dpi, brightness, contrast, filename);
}

int slScanImage(unsigned verbose, unsigned dpi, int brightness, int contrast,
		double left, double top, double right, double bottom,
		const char * filename) {
	DmScanLib dmScanLib;
	return dmScanLib.scanImage(verbose, dpi, brightness, contrast, left, top,
			right, bottom, filename);
}

int slDecodePlate(unsigned verbose, unsigned dpi, int brightness, int contrast,
		unsigned plateNum, double left, double top, double right, double bottom,
		double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance, double gapX, double gapY,
		unsigned profileA, unsigned profileB, unsigned profileC,
		unsigned orientation) {
	DmScanLib dmScanLib;
	dmScanLib.setTextFileOutputEnable(true);
	return dmScanLib.decodePlate(verbose, dpi, brightness, contrast, plateNum,
			left, top, right, bottom, scanGap, squareDev, edgeThresh,
			corrections, cellDistance, gapX, gapY, profileA, profileB, profileC,
			false, orientation);
}

int slDecodeImage(unsigned verbose, unsigned plateNum, const char * filename,
		double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance, double gapX, double gapY,
		unsigned profileA, unsigned profileB, unsigned profileC,
		unsigned orientation) {
	DmScanLib dmScanLib;
	dmScanLib.setTextFileOutputEnable(true);
	return dmScanLib.decodeImage(verbose, plateNum, filename, scanGap,
			squareDev, edgeThresh, corrections, cellDistance, gapX, gapY,
			profileA, profileB, profileC,false, orientation);
}

