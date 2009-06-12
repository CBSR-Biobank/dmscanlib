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
short slIsTwainAvailable() {
	ImageGrabber ig;
	if (ig.twainAvailable()) {
		return SC_SUCCESS;
	}
	return SC_TWAIN_UAVAIL;
}

short slSelectSourceAsDefault() {
	ImageGrabber ig;
	if (ig.selectSourceAsDefault()) {
		return SC_SUCCESS;
	}
	return SC_FAIL;
}

short slScanImage(char * filename, double left, double top, double right,
		double bottom) {
	string err;
	ImageGrabber ig;

	if (filename == NULL) {
		return SC_FAIL;
	}

	configLogging(5);
	HANDLE h = ig.acquireImage(left, top, right, bottom);
	if (h == NULL) {
		return SC_FAIL;
	}
	Dib dib;
	dib.readFromHandle(h);
	dib.writeToFile(filename);
	ig.freeImage(h);
	return SC_SUCCESS;
}

short slConfigPlateFrame(unsigned short plateNum, double left,
		double top,	double right, double bottom) {
	Config config(INI_FILE_NAME);
	if (!config.savePlateFrame(plateNum, left, top, right, bottom)) {
		return SC_INI_FILE_ERROR;
	}
	return SC_SUCCESS;
}

short slCalibrateToPlate(unsigned short plateNum) {
	if (plateNum > 4) {
		UA_DOUT(1, 1, "plate number is invalid: " << plateNum);
		return SC_FAIL;
	}

	configLogging(5);

	Config config(INI_FILE_NAME);
	ScFrame * f = NULL;

	config.parseFrames();
	if (!config.getPlateFrame(plateNum, &f)) {
		return SC_INI_FILE_ERROR;
	}

	Calibrator calibrator;
	Dib dib;
	RgbQuad quad(255, 0, 0);

	ImageGrabber ig;
	HANDLE h = ig.acquireImage(f->x0, f->y0, f->x1, f->y1);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not aquire plate image: " << plateNum);
		return SC_FAIL;
	}
	dib.readFromHandle(h);
	dib.writeToFile("out.bmp");
	if (!calibrator.processImage(dib)) {
		return SC_CALIBRATOR_NO_REGIONS;
	}

	if (!config.setRegions(plateNum, calibrator.getRowBinRegions(),
			calibrator.getColBinRegions(), calibrator.getMaxCol())) {
		return SC_INI_FILE_ERROR;
	}

	Dib markedDib(dib);

	calibrator.imageShowBins(markedDib, quad);
	markedDib.writeToFile("marked.bmp");
	ig.freeImage(h);
	return SC_SUCCESS;
}

void saveDecodeResults(unsigned plateNum, vector<DecodeRegion *> & decodeRegions) {
	ofstream file;
	file.open("scanlib.txt", ios::out);

	file << "#Plate,Row,Col,Barcode" << endl;

	for (unsigned i = 0, n = decodeRegions.size(); i < n; ++i) {
		DecodeRegion & region = *decodeRegions[i];

		if (region.msgInfo == NULL) continue;

		file << to_string(plateNum) << ","
		     << to_string(region.row) << ","
		     << to_string(region.col) << ","
		     << region.msgInfo->getMsg() << endl;
	}
	file.close();
}

short slDecodePlate(unsigned short plateNum) {
	if (plateNum > 4) {
		UA_DOUT(1, 1, "plate number is invalid: " << plateNum);
		return SC_FAIL;
	}

	configLogging(5);

	Config config(INI_FILE_NAME);
	ScFrame * f;

	config.parseFrames();
	if (config.getPlateFrame(plateNum, &f)) {
		return SC_INI_FILE_ERROR;
	}
	if (!config.parseRegions(plateNum)) {
		return SC_INI_FILE_ERROR;
	}

	Decoder decoder;
	Dib dib;

	ImageGrabber ig;
	HANDLE h = ig.acquireImage(f->x0, f->y0, f->x1, f->y1);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not acquire plate image: " << plateNum);
		return SC_FAIL;
	}
	dib.readFromHandle(h);
	decoder.processImageRegions(plateNum, dib, config.getRegions());
	saveDecodeResults(plateNum, config.getRegions());

	Dib markedDib(dib);
	RgbQuad quad(0, 255, 0);

	decoder.imageShowRegions(markedDib, quad);
	markedDib.writeToFile("marked.bmp");
	ig.freeImage(h);
	return SC_SUCCESS;
}
