/*******************************************************************************
 * Canadian Biosample Repository
 *
 * ScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 ******************************************************************************/

#include "ScanLib.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Decoder.h"
#include "Calibrator.h"
#include "Dib.h"
#include "Util.h"

#ifdef WIN32
#include "ImageGrabber.h"
#endif

#define SI_SUPPORT_IOSTREAMS
#include "SimpleIni.h"

#include <iostream>
#include <fstream>

using namespace std;

const char * INI_FILE_NAME = "scanlib.ini";

CSimpleIniA ini(true, false, true);

/*
 * Loads the INI file if it is present.
 */
unsigned short slIsTwainAvailable() {
	ImageGrabber ig;
	if (ig.twainAvailable()) {
		return SC_SUCCESS;
	}
	return SC_TWAIN_UAVAIL;
}

void calibrateToImage(char * filename) {
	UA_ASSERT_NOT_NULL(filename);

	Dib dib;
	RgbQuad quad(255, 0, 0);

	dib.readFromFile(filename);
	Dib markedDib(dib);
	Calibrator calibrator;
	calibrator.processImage(dib);
	calibrator.saveRegionsToIni(ini);
	ini.SaveFile(INI_FILE_NAME);

	calibrator.imageShowBins(markedDib, quad);
	markedDib.writeToFile("out.bmp");
}

void decodeImage(char * filename) {
	UA_ASSERT_NOT_NULL(filename);

	Dib dib;
	RgbQuad quad(255, 0, 0);
	//DmtxVector2 p00, p10, p11, p01;

	dib.readFromFile(filename);
	Dib markedDib(dib);
	Decoder decoder;
	decoder.getRegionsFromIni(ini);
	decoder.processImageRegions(dib);
	markedDib.writeToFile("out.bmp");
}

void acquireAndProcesImage(unsigned plateNum) {
#ifdef WIN32
	string err;
	ImageGrabber ig;

	ig.getConfigFromIni(ini, err);

	HANDLE h = ig.acquirePlateImage(err, plateNum);
	if (h == NULL) {
		cerr <<  err << endl;
		exit(0);
	}

	Dib dib;
	Decoder decoder;

	dib.readFromHandle(h);
	ig.freeImage(h);
	dib.writeToFile("out.bmp");
#else
	cerr << "this option not allowed on your operating system." << endl;
#endif
}

unsigned short scanImage(char * filename, double left, double top, double right,
		double bottom) {
#ifdef WIN32
	string err;
	ImageGrabber ig;

	ua::logstream.sink(ua::LoggerSinkStdout::Instance());
	ua::LoggerSinkStdout::Instance().showHeader(true);
	ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m, 5);

	if (filename == NULL) {
		return SC_FAIL;
	}
	HANDLE h = ig.acquireImage(err, left, top, right, bottom);
	if (h == NULL) {
		return SC_FAIL;
	}
	Dib dib;
	dib.readFromHandle(h);
	dib.writeToFile(filename);
	ig.freeImage(h);
	return SC_SUCCESS;
#else
	cerr << "this option not allowed on your operating system." << endl;
#endif
}
