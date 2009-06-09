/*******************************************************************************
 * Canadian Biosample Repository
 *
 * ScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 ******************************************************************************/

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
bool initialize() {
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

	ImageGrabber::Instance().getConfigFromIni(ini, err);

	HANDLE h = ImageGrabber::Instance().acquirePlateImage(err, plateNum);
	if (h == NULL) {
		cerr <<  err << endl;
		exit(0);
	}

	Dib dib;
	Decoder decoder;

	dib.readFromHandle(h);
	ImageGrabber::Instance().freeImage(h);
	dib.writeToFile("out.bmp");
#else
	cerr << "this option not allowed on your operating system." << endl;
#endif
}

void scanImage(char * filename) {
#ifdef WIN32
	string err;

	UA_ASSERT_NOT_NULL(filename);
	HANDLE h = ImageGrabber::Instance().acquireImage(err, 0, 0, 0, 0);
	if (h == NULL) {
		cerr <<  err << endl;
		exit(0);
	}
	Dib dib;
	dib.readFromHandle(h);
	dib.writeToFile(filename);
	ImageGrabber::Instance().freeImage(h);
#else
	cerr << "this option not allowed on your operating system." << endl;
#endif
}
