/*
Dmscanlib is a software library and standalone application that scans 
and decodes libdmtx compatible test-tubes. It is currently designed 
to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
Copyright (C) 2010 Canadian Biosample Repository

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*******************************************************************************
 * Canadian Biosample Repository
 *
 * DmScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 ******************************************************************************/

#include "DmScanLib.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Decoder.h"
#include "Dib.h"
#include "Util.h"
#include "BarcodeInfo.h"
#include "structs.h"

#ifdef WIN32
#include "ImageGrabber.h"
#endif

#ifdef _VISUALC_
// disable warnings about fopen
#pragma warning(disable : 4996)
#endif

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <ctime>
#include <cstdlib>
#include <math.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

using namespace std;

const char *INI_FILE_NAME = "dmscanlib.ini";

slTime starttime; // for debugging
slTime endtime;
slTime timediff;

static bool loggingInitialized = false;

void configLogging(unsigned level, bool useFile = true) {
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

/*
 * Could not use C++ streams for Release version of DLL.
 */
void saveResults(string & msg) {
	FILE *fh = fopen("dmscanlib.txt", "w");
	UA_ASSERT_NOT_NULL(fh);
	fprintf(fh, "%s", msg.c_str());
	fclose(fh);
}

int slIsTwainAvailable() {
#ifdef WIN32
	ImageGrabber ig;
	if (ig.twainAvailable()) {
		return SC_SUCCESS;
	}
#endif
	return SC_TWAIN_UAVAIL;
}

int slSelectSourceAsDefault() {
#ifdef WIN32
	ImageGrabber ig;
	if (ig.selectSourceAsDefault()) {
		return SC_SUCCESS;
	}
#endif
	return SC_FAIL;
}

/*
 * Please note that the 32nd bit should be ignored. 
 */
int slGetScannerCapability() {
#ifdef WIN32
	ImageGrabber ig;
	return ig.getScannerCapability();
#endif
	return 0xFF; // supports WIA and DPI: 300,400,600
}

int isValidDpi(int dpi) {
#ifdef WIN32
	ImageGrabber ig;
	int dpiCap = ig.getScannerCapability();
	return ((dpiCap & CAP_DPI_300) && dpi == 300)
		|| ((dpiCap & CAP_DPI_400) && dpi == 400)
		|| ((dpiCap & CAP_DPI_600) && dpi == 600);
#else
	return 1;
#endif
}

void formatCellMessages(unsigned plateNum, vector<vector<string> >&cells,
		string & msg) {
	ostringstream out;
	out << "#Plate,Row,Col,Barcode" << endl;
	for (unsigned row = 0, numRows = cells.size(); row < numRows; ++row) {
		for (unsigned col = 0, numCols = cells[row].size(); col < numCols; ++col) {
			if (cells[row][col].length() == 0)
				continue;

			out << plateNum << "," << (char) ('A' + row) << "," << col + 1
					<< "," << cells[row][col] << endl;
		}
	}
	msg = out.str();
}

int slScanImage(unsigned verbose, unsigned dpi, int brightness, int contrast,
		double left, double top, double right, double bottom,
		const char *filename) {
	configLogging(verbose);
	UA_DOUT(1, 3, "slScanImage: dpi/" << dpi
			<< " brightness/" << brightness
			<< " contrast/" << contrast
			<< " left/" << left
			<< " top/" << top << " right/" << right << " bottom/" <<
			bottom);

#ifdef WIN32
	ImageGrabber ig;

	if (filename == NULL) {
		return SC_FAIL;
	}

	HANDLE h = ig.acquireImage(dpi, brightness, contrast, left, top, right,
			bottom);
	if (h == NULL) {
		return ig.getErrorCode();
	}
	Dib dib;
	dib.readFromHandle(h);
	dib.writeToFile(filename);
	ig.freeImage(h);
	return SC_SUCCESS;
#else
	return SC_FAIL;
#endif
}

int slDecodeCommon(unsigned plateNum, Dib & dib, Decoder & decoder,
		const char *markedDibFilename) {

	bool metrical = false;
	Dib *filteredDib;
	Decoder::ProcessResult result;

	UA_DOUT(1, 5, "DecodeCommon: metrical mode: " << metrical);

	/*--- apply filters ---*/
	filteredDib = Dib::convertGrayscale(dib);
	UA_ASSERT_NOT_NULL(filteredDib);

	filteredDib->tpPresetFilter();
	UA_ASSERT_NOT_NULL(filteredDib);

	UA_DEBUG(
			filteredDib->writeToFile("filtered.bmp"));

	/*--- obtain barcodes ---*/
	result = decoder.processImageRegions(filteredDib);

	delete filteredDib;

	decoder.imageShowBarcodes(dib, 0);
	if (result == Decoder::OK) 
		dib.writeToFile(markedDibFilename);
	else
		dib.writeToFile("decode.partial.bmp");


	switch (result) {
	case Decoder::IMG_INVALID:
		return SC_INVALID_IMAGE;

	case Decoder::POS_INVALID:
		return SC_INVALID_POSITION;

	case Decoder::POS_CALC_ERROR:
		return SC_POS_CALC_ERROR;

	default:
		; // do nothing
	}

	// only get here if decoder returned Decoder::OK
	Util::getTime(endtime);
	Util::difftiime(starttime, endtime, timediff);
	UA_DOUT(1, 1, "slDecodeCommonCv: time taken: " << timediff);
	return SC_SUCCESS;
}

int slDecodePlate(unsigned verbose, unsigned dpi, int brightness, int contrast,
		unsigned plateNum, double left, double top, double right,
		double bottom, double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance, double gapX, double gapY,
		unsigned profileA,unsigned profileB, unsigned profileC, unsigned isHorizontal) {
	configLogging(verbose);
	UA_DOUT(1, 3, "slDecodePlate: dpi/" << dpi
			<< " brightness/" << brightness
			<< " contrast/" << contrast
			<< " plateNum/" << plateNum
			<< " left/" << left
			<< " top/" << top
			<< " right/" << right
			<< " bottom/" << bottom
			<< " scanGap/" << scanGap
			<< " squareDev/" << squareDev
			<< " edgeThresh/" << edgeThresh
			<< " corrections/" << corrections
			<< " cellDistance/" << cellDistance
			<< " gapX/" << gapX
			<< " gapY/" << gapY
			<< " isHorizontal/" << isHorizontal);

#ifdef WIN32
	ImageGrabber ig;

	if ((plateNum < MIN_PLATE_NUM) || (plateNum > MAX_PLATE_NUM)) {
		return SC_INVALID_PLATE_NUM;
	}

	HANDLE h;
	int result;
	Dib dib;
	vector < vector < string > >cells;
	Util::getTime(starttime);
	Decoder decoder(scanGap, squareDev, edgeThresh, corrections,
			cellDistance,gapX,gapY,profileA,profileB,profileC,isHorizontal);

	h = ig.acquireImage(dpi, brightness, contrast, left, top, right,
			bottom);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not acquire plate image: " << plateNum);
		return ig.getErrorCode();
	}

	dib.readFromHandle(h);
	dib.writeToFile("scanned.bmp");

	if (dib.getDpi() != dpi) {
		result = SC_INCORRECT_DPI_SCANNED;
	} else {
		result = slDecodeCommon(plateNum, dib, decoder, "decode.bmp");
	}

	if (result == SC_SUCCESS) {
		string msg;
		formatCellMessages(plateNum, decoder.getCells(), msg);
		saveResults(msg);
	}

	ig.freeImage(h);
	return result;
#else
	return SC_FAIL;
#endif
}

int slDecodeImage(unsigned verbose, unsigned plateNum, const char *filename,
		double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance, double gapX, double gapY,
		unsigned profileA,unsigned profileB, unsigned profileC, unsigned isHorizontal) {
	configLogging(verbose);
	UA_DOUT(1, 3, "slDecodeImage: plateNum/" << plateNum
			<< " filename/" << filename
			<< " scanGap/" << scanGap
			<< " squareDev/" << squareDev
			<< " edgeThresh/" << edgeThresh
			<< " corrections/" << corrections
			<< " cellDistance/" << cellDistance
			<< " gapX/" << gapX
			<< " gapY/" << gapY
			<< " isHorizontal/" << isHorizontal);

	if ((plateNum < MIN_PLATE_NUM) || (plateNum > MAX_PLATE_NUM)) {
		return SC_INVALID_PLATE_NUM;
	}

	if (filename == NULL) {
		return SC_FAIL;
	}

	Util::getTime(starttime);

	Dib dib;
	Decoder decoder(scanGap, squareDev, edgeThresh, corrections, cellDistance,
		gapX,gapY,profileA,profileB,profileC,isHorizontal);

	dib.readFromFile(filename);

	int result = slDecodeCommon(plateNum, dib, decoder, "decode.bmp");

	if (result == SC_SUCCESS) {
		string msg;
		formatCellMessages(plateNum, decoder.getCells(), msg);
		saveResults(msg);
	}
	return result;
}
