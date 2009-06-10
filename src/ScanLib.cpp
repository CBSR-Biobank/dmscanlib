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
bool iniLoad(CSimpleIniA & ini) {
	fstream inifile;
	inifile.open(INI_FILE_NAME, fstream::in);
	if (!inifile.is_open()) return false;

	SI_Error rc = ini.Load(inifile);
	if (rc >= 0) {
		// attempt to load ini file failed
		return false;
	}
	inifile.close();
	return true;
}

void configLogging(unsigned level) {
	ua::logstream.sink(ua::LoggerSinkStdout::Instance());
	ua::LoggerSinkStdout::Instance().showHeader(true);
	ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m, level);
}

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

unsigned short slSelectSourceAsDefault() {
	ImageGrabber ig;
	if (ig.selectSourceAsDefault()) {
		return SC_SUCCESS;
	}
	return SC_FAIL;
}

unsigned short slScanImage(char * filename, double left, double top, double right,
		double bottom) {
#ifdef WIN32
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
#else
	cerr << "this option not allowed on your operating system." << endl;
#endif
}

unsigned short slConfigPlateFrame(unsigned short plateNum, double left,
		double top,	double right, double bottom) {
	CSimpleIniA ini(true, false, true);
	SI_Error rc;

	iniLoad(ini);
	string secname = "plate-" + to_string(plateNum);

	rc = ini.SetValue(secname.c_str(), "left", to_string(left).c_str());
	if (rc < 0) return SC_FAIL;

	rc = ini.SetValue(secname.c_str(), "top", to_string(top).c_str());
	if (rc < 0) return SC_FAIL;

	rc = ini.SetValue(secname.c_str(), "right", to_string(right).c_str());
	if (rc < 0) return SC_FAIL;

	rc = ini.SetValue(secname.c_str(), "bottom", to_string(bottom).c_str());
	if (rc < 0) return SC_FAIL;

	ini.SaveFile(INI_FILE_NAME);
	return SC_SUCCESS;
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
	ImageGrabber ig;

	ig.getConfigFromIni(ini);

	HANDLE h = ig.acquirePlateImage(plateNum);
	if (h == NULL) {
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
