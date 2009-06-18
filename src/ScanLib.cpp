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

#ifdef WIN32
#include "ImageGrabber.h"
#endif

#define SI_SUPPORT_IOSTREAMS
#include "SimpleIni.h"

#include <iostream>
#include <fstream>

using namespace std;

const char * INI_FILE_NAME = "scanlib.ini";

void configLogging(unsigned level) {
	ua::logstream.sink(ua::LoggerSinkStdout::Instance());
	ua::LoggerSinkStdout::Instance().showHeader(true);
	ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m, level);
	ua::Logger::Instance().subSysHeaderSet(1, "ScanLib");
}

/*
 * Loads the INI file if it is present.
 */
int slIsTwainAvailable() {
	ImageGrabber ig;
	if (ig.twainAvailable()) {
		return SC_SUCCESS;
	}
	return SC_TWAIN_UAVAIL;
}

int slSelectSourceAsDefault() {
	ImageGrabber ig;
	if (ig.selectSourceAsDefault()) {
		return SC_SUCCESS;
	}
	return SC_FAIL;
}

int slScanImage(unsigned dpi, double left, double top, double right,
		double bottom, char * filename) {
	string err;
	ImageGrabber ig;

	if (filename == NULL) {
		return SC_FAIL;
	}

	configLogging(5);
	HANDLE h = ig.acquireImage(dpi, left, top, right, bottom);
	if (h == NULL) {
		return SC_FAIL;
	}
	Dib dib;
	dib.readFromHandle(h);
	dib.writeToFile(filename);
	ig.freeImage(h);
	return SC_SUCCESS;
}

int slConfigPlateFrame(unsigned plateNum, double left,
		double top,	double right, double bottom) {
	Config config(INI_FILE_NAME);
	if (!config.savePlateFrame(plateNum, left, top, right, bottom)) {
		return SC_INI_FILE_ERROR;
	}
	return SC_SUCCESS;
}

int slCalibrateToPlate(unsigned dpi, unsigned plateNum) {
	if ((dpi != 300) && (dpi != 400) && (dpi != 600)) {
		return SC_INVALID_DPI;
	}

	if ((plateNum == 0) || (plateNum > 4)) {
		return SC_INVALID_PLATE_NUM;
	}

	slTime starttime; // for debugging
	UA_DEBUG(
		Util::getTime(starttime);
	);

	configLogging(5);

	Config config(INI_FILE_NAME);
	ScFrame * f = NULL;
	Calibrator calibrator;
	Dib dib;
	Dib processedDib;
	RgbQuad quad(255, 0, 0);
	ImageGrabber ig;
	HANDLE h;

	config.parseFrames();
	if (!config.getPlateFrame(plateNum, &f)) {
		return SC_INI_FILE_ERROR;
	}

	h = ig.acquireImage(dpi, f->x0, f->y0, f->x1, f->y1);
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

	UA_DEBUG(
		slTime endtime;
		slTime difftime;
		Util::getTime(endtime);
		Util::difftiime(starttime, endtime, difftime);
		UA_DOUT(1, 1, "slDecodePlate: time taken: " << difftime);
	);

	return SC_SUCCESS;
}

int slDecodePlate(unsigned dpi, unsigned plateNum) {
	if ((dpi != 300) && (dpi != 400) && (dpi != 600)) {
		return SC_INVALID_DPI;
	}

	if ((plateNum == 0) || (plateNum > 4)) {
		return SC_INVALID_PLATE_NUM;
	}

	slTime starttime; // for debugging
	UA_DEBUG(
		Util::getTime(starttime);
	);

	configLogging(5);

	Config config(INI_FILE_NAME);
	ScFrame * f;
	Decoder decoder;
	Dib dib, hdib;
	Dib processedDib;
	ImageGrabber ig;
	HANDLE h;

	config.parseFrames();
	if (!config.getPlateFrame(plateNum, &f)) {
		return SC_INI_FILE_ERROR;
	}
	if (!config.parseRegions(plateNum)) {
		return SC_INI_FILE_ERROR;
	}

	h = ig.acquireImage(dpi, f->x0, f->y0, f->x1, f->y1);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not acquire plate image: " << plateNum);
		return SC_FAIL;
	}
	dib.readFromHandle(h);
	hdib.histEqualization(dib);
	hdib.writeToFile("histeq.bmp");
	processedDib.gaussianBlur(dib);
	processedDib.unsharp(dib);
	processedDib.expandColours(150, 230);
	processedDib.writeToFile("processed.bmp");
	dib.writeToFile("scanned.bmp");

	const vector<DecodeRegion *> & regions = config.getRegions(plateNum, dpi);

	decoder.processImageRegions(plateNum, processedDib, regions);
	//decoder.processImageRegions(plateNum, dib, config.getRegions());
	config.saveDecodeResults(plateNum);

	Dib markedDib(dib);

	decoder.imageShowRegions(markedDib, regions);
	markedDib.writeToFile("decoded.bmp");
	ig.freeImage(h);

	UA_DEBUG(
		slTime endtime;
		slTime difftime;
		Util::getTime(endtime);
		Util::difftiime(starttime, endtime, difftime);
		UA_DOUT(1, 1, "slDecodePlate: time taken: " << difftime);
	);

	return SC_SUCCESS;
}
