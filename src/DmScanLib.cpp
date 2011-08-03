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
#include "UaLogger.h"
#include "UaAssert.h"
#include "Decoder.h"
#include "Dib.h"


#include <stdio.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

//using namespace std;

bool DmScanLib::loggingInitialized = false;

DmScanLib::DmScanLib(unsigned debugLevel, bool haveDebugFile) {
	configLogging(debugLevel, haveDebugFile);
}

DmScanLib::~DmScanLib() {

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

/*
 * Could not use C++ streams for Release version of DLL.
 */
void DmScanLib::saveResults(string & msg) {
	FILE *fh = fopen("dmscanlib.txt", "w");
	UA_ASSERT_NOT_NULL(fh);
	fprintf(fh, "%s", msg.c_str());
	fclose(fh);
}

void DmScanLib::formatCellMessages(unsigned plateNum, Decoder & decoder, string & msg) {
	ostringstream out;
	out << "#Plate,Row,Col,Barcode" << endl;

	for (unsigned row = 0; row < PalletGrid::MAX_ROWS; ++row) {
		for (unsigned col = 0; col < PalletGrid::MAX_COLS; ++col) {
			const char * msg = decoder.getBarcode(row, col);
			if (msg == NULL
				) continue;
			out << plateNum << "," << static_cast<char>('A' + row) << ","
					<< (col + 1) << "," << msg << endl;
		}
	}
	msg = out.str();
}

int DmScanLib::decodeCommon(unsigned plateNum, Dib & dib, double scanGap,
		unsigned squareDev, unsigned edgeThresh, unsigned corrections,
		double cellDistance, double gapX, double gapY, unsigned profileA,
		unsigned profileB, unsigned profileC, unsigned isVertical,
		const char *markedDibFilename) {

	unsigned dpi = dib.getDpi();
	UA_DOUT(1, 3, "DecodeCommon: dpi/" << dpi);
	if ((dpi != 300) && (dpi != 400) && (dpi != 600)) {
		return SC_INVALID_DPI;
	}

	Decoder::ProcessResult result;

	PalletGrid::Orientation orientation = (
			isVertical ?
					PalletGrid::ORIENTATION_VERTICAL :
					PalletGrid::ORIENTATION_HORIZONTAL);

	unsigned gapXpixels = static_cast<unsigned>(dpi * gapX);unsigned
	gapYpixels = static_cast<unsigned>(dpi * gapY);

	const	unsigned profileWords[3] = { profileA, profileB, profileC };

	auto_ptr<PalletGrid> palletGrid(
			new PalletGrid(orientation, dib.getWidth(), dib.getHeight(),
					gapXpixels, gapYpixels, profileWords));

	if (!palletGrid->isImageValid()) {
		return SC_INVALID_IMAGE;
	}

	Decoder decoder(scanGap, squareDev, edgeThresh, corrections, cellDistance,
			palletGrid.get());

	/*--- apply filters ---*/
	auto_ptr<Dib> filteredDib(Dib::convertGrayscale(dib));

	filteredDib->tpPresetFilter();
	UA_DEBUG( filteredDib->writeToFile("filtered.bmp"));

	/*--- obtain barcodes ---*/
	result = decoder.processImageRegions(filteredDib.get());

	decoder.imageShowBarcodes(dib, 0);
	if (result == Decoder::OK)
		dib.writeToFile(markedDibFilename);
	else
		dib.writeToFile("decode.partial.bmp");

	switch (result) {
	case Decoder::IMG_INVALID:
		return SC_INVALID_IMAGE;

	default:
		; // do nothing
	}

	// only get here if decoder returned Decoder::OK
	string msg;
	formatCellMessages(plateNum, decoder, msg);
	saveResults(msg);

	Util::getTime(endtime);
	Util::difftiime(starttime, endtime, timediff);
	UA_DOUT(1, 1, "slDecodeCommonCv: time taken: " << timediff);
	return SC_SUCCESS;
}

int DmScanLib::decodeImage(unsigned verbose, unsigned plateNum, const char *filename,
		double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance, double gapX, double gapY,
		unsigned profileA, unsigned profileB, unsigned profileC,
		unsigned isVertical) {
	configLogging(verbose);
	UA_DOUT(
			1,
			3,
			"slDecodeImage: plateNum/" << plateNum << " filename/" << filename << " scanGap/" << scanGap << " squareDev/" << squareDev << " edgeThresh/" << edgeThresh << " corrections/" << corrections << " cellDistance/" << cellDistance << " gapX/" << gapX << " gapY/" << gapY << " isVertical/" << isVertical);

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
			isVertical, "decode.bmp");
	return result;
}
