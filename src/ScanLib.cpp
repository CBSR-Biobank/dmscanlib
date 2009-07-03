/*******************************************************************************
 * Canadian Biosample Repository
 *
 * ScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 ******************************************************************************/

#include "ScanLib.h"
#include "Config.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Decoder.h"
#include "Calibrator.h"
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

#define SI_SUPPORT_IOSTREAMS
#include "SimpleIni.h"

#include <iostream>
#include <fstream>

using namespace std;

const char * INI_FILE_NAME = "scanlib.ini";

slTime starttime; // for debugging
slTime endtime;
slTime timediff;

void configLogging(unsigned level) {
	ua::logstream.sink(ua::LoggerSinkStdout::Instance());
	ua::LoggerSinkStdout::Instance().showHeader(true);
	ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m, level);
	ua::Logger::Instance().subSysHeaderSet(1, "ScanLib");
}


/*
 * Could not use C++ streams for Release version of DLL.
 */
void saveDecodeResults(unsigned plateNum, const vector<DecodeRegion *> & regions) {
	UA_ASSERTS((plateNum > 0) && (plateNum <= Config::MAX_PLATES),
			"parseRegions: invalid plate number: " << plateNum);
	FILE * fh = fopen("scanlib.txt", "w");
    UA_ASSERT_NOT_NULL(fh);
    fprintf(fh, "#Plate,Row,Col,Barcode\n");

	for (unsigned i = 0, n = regions.size(); i < n; ++i) {
		DecodeRegion & region = *regions[i];

		if (region.barcodeInfo == NULL) continue;

		fprintf(fh, "%d,%c,%d,%s\n",
				plateNum, static_cast<char>('A' + region.row),
				region.col + 1, region.barcodeInfo->getMsg().c_str());
	}
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

int slConfigScannerBrightness(int brightness) {
	if ((brightness < -1000) || (brightness > 1000)) {
		return SC_INVALID_VALUE;
	}

	configLogging(5);

	Config config(INI_FILE_NAME);
	if (!config.setScannerBrightness(brightness)) {
		return SC_INI_FILE_ERROR;
	}
	return SC_SUCCESS;
}

int slConfigScannerContrast(int contrast) {
	if ((contrast < -1000) || (contrast > 1000)) {
		return SC_INVALID_VALUE;
	}

	configLogging(5);

	Config config(INI_FILE_NAME);
	if (!config.setScannerContrast(contrast)) {
		return SC_INI_FILE_ERROR;
	}
	return SC_SUCCESS;
}

int slConfigPlateFrame(unsigned plateNum, double left,
		double top,	double right, double bottom) {
	Config config(INI_FILE_NAME);
	if (!config.setPlateFrame(plateNum, left, top, right, bottom)) {
		return SC_INI_FILE_ERROR;
	}
	return SC_SUCCESS;
}

int slScanImage(unsigned dpi, double left, double top, double right,
		double bottom, char * filename) {
#ifdef WIN32
	if (filename == NULL) {
		return SC_FAIL;
	}

	configLogging(5);

	Config config(INI_FILE_NAME);
	ImageGrabber ig;

	HANDLE h = ig.acquireImage(dpi, config.getScannerBrightness(),
		config.getScannerContrast(), left, top, right, bottom);
	if (h == NULL) {
		return SC_FAIL;
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

int slScanPlate(unsigned dpi, unsigned plateNum, char * filename) {
#ifdef WIN32
	if ((dpi != 300) && (dpi != 400) && (dpi != 600)) {
		return SC_INVALID_DPI;
	}

	if ((plateNum == 0) || (plateNum > 4)) {
		return SC_INVALID_PLATE_NUM;
	}

	if (filename == NULL) {
		return SC_FAIL;
	}

	ScFrame f;
	ImageGrabber ig;
	HANDLE h;
	Dib dib;

	UA_DEBUG(
		Util::getTime(starttime);
	);

	configLogging(5);
	Config config(INI_FILE_NAME);

	config.parseFrames();
	if (!config.getPlateFrame(plateNum, f)) {
		return SC_INI_FILE_ERROR;
	}

	h = ig.acquireImage(dpi, config.getScannerBrightness(),
		config.getScannerContrast(), f.x0, f.y0, f.x1, f.y1);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not acquire plate image: " << plateNum);
		return SC_FAIL;
	}

	dib.readFromHandle(h);
	if (!dib.writeToFile(filename)) {
		return SC_FILE_SAVE_ERROR;
	}
	ig.freeImage(h);
	return SC_SUCCESS;
#else
	return SC_FAIL;
#endif
}

int slCalibrateToPlate(unsigned dpi, unsigned plateNum) {
#ifdef WIN32
	if ((dpi != 300) && (dpi != 400) && (dpi != 600)) {
		return SC_INVALID_DPI;
	}

	if ((plateNum == 0) || (plateNum > 4)) {
		return SC_INVALID_PLATE_NUM;
	}

	ScFrame f;
	Calibrator calibrator;
	Dib dib;
	Dib processedDib;
	RgbQuad quad(255, 0, 0);
	ImageGrabber ig;
	HANDLE h;
	Config config(INI_FILE_NAME);

	Util::getTime(starttime);

	configLogging(5);

	config.parseFrames();
	if (!config.getPlateFrame(plateNum, f)) {
		return SC_INI_FILE_ERROR;
	}

	h = ig.acquireImage(dpi, config.getScannerBrightness(),
		config.getScannerContrast(), f.x0, f.y0, f.x1, f.y1);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not aquire plate image: " << plateNum);
		return SC_FAIL;
	}
	dib.readFromHandle(h);
	dib.writeToFile("scanned.bmp");
	processedDib.gaussianBlur(dib);
	processedDib.unsharp(dib);
	processedDib.expandColours(150, 220);
	processedDib.writeToFile("processed.bmp");

	if (!calibrator.processImage(processedDib)) {
		return SC_CALIBRATOR_NO_REGIONS;
	}

	const vector<BinRegion*> & rowBins = calibrator.getRowBinRegions();
	const vector<BinRegion*> & colBins = calibrator.getColBinRegions();

	if ((rowBins.size() != 8) || (colBins.size() != 12)) {
		return SC_CALIBRATOR_ERROR;
	}

	if (!config.setRegions(plateNum, dpi, rowBins, colBins)) {
		return SC_INI_FILE_ERROR;
	}

	Dib markedDib(dib);

	calibrator.imageShowBins(markedDib, quad);
	markedDib.writeToFile("calibrated.bmp");
	ig.freeImage(h);

	Util::getTime(endtime);
	Util::difftiime(starttime, endtime, timediff);
	UA_DOUT(1, 1, "slDecodePlate: time taken: " << timediff);

	return SC_SUCCESS;
#else
	return SC_FAIL;
#endif
}

int slDecodeCommon(unsigned plateNum, Dib & dib) {
	Decoder decoder;
	Config config(INI_FILE_NAME);

 	if (!config.parseRegions(plateNum)) {
		return SC_INI_FILE_ERROR;
	}

	//processedDib.gaussianBlur(dib);
	//processedDib.unsharp(dib);
	Dib processedDib(dib);
	processedDib.expandColours(100, 200);
	processedDib.writeToFile("processed.bmp");
	dib.writeToFile("scanned.bmp");

	const vector<DecodeRegion *> & regions = config.getRegions(plateNum, dib.getDpi());

	decoder.processImageRegions(plateNum, processedDib, regions);
	//decoder.processImageRegions(plateNum, dib, regions);
	saveDecodeResults(plateNum, regions);

	Dib markedDib(dib);

	decoder.imageShowRegions(markedDib, regions);
	markedDib.writeToFile("decoded.bmp");

	Util::getTime(endtime);
	Util::difftiime(starttime, endtime, timediff);
	UA_DOUT(1, 1, "slDecodePlate: time taken: " << timediff);

	return SC_SUCCESS;
}

int slDecodePlate(unsigned dpi, unsigned plateNum) {
#ifdef WIN32
	if ((dpi != 300) && (dpi != 400) && (dpi != 600)) {
		return SC_INVALID_DPI;
	}

	if ((plateNum == 0) || (plateNum > 4)) {
		return SC_INVALID_PLATE_NUM;
	}

	ScFrame f;
	ImageGrabber ig;
	HANDLE h;
	int result;
	Dib dib;

	Util::getTime(starttime);
	Config config(INI_FILE_NAME);
	configLogging(5);
	config.parseFrames();
	if (!config.getPlateFrame(plateNum, f)) {
		return SC_INI_FILE_ERROR;
	}

	h = ig.acquireImage(dpi, config.getScannerBrightness(),
		config.getScannerContrast(), f.x0, f.y0, f.x1, f.y1);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not acquire plate image: " << plateNum);
		return SC_FAIL;
	}

	dib.readFromHandle(h);
	result = slDecodeCommon(plateNum, dib);
	ig.freeImage(h);
	return result;
#else
	return SC_FAIL;
#endif
}

int slDecodeImage(unsigned plateNum, char * filename) {
	if ((plateNum == 0) || (plateNum > 4)) {
		return SC_INVALID_PLATE_NUM;
	}

	if (filename == NULL) {
		return SC_FAIL;
	}

	Util::getTime(starttime);

	Dib dib;

	configLogging(5);

	dib.readFromFile(filename);
	return slDecodeCommon(plateNum, dib);
}
