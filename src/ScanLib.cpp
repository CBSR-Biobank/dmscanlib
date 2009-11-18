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

using namespace std;

const char * INI_FILE_NAME = "scanlib.ini";

slTime starttime; // for debugging
slTime endtime;
slTime timediff;

static bool loggingInitialized = false;

void configLogging(bool useFile, unsigned level) {
	if (loggingInitialized)
		return;

	if (useFile) {
		ua::LoggerSinkFile::Instance().setFile("scanlib.log");
		ua::LoggerSinkFile::Instance().showHeader(true);
		ua::logstream.sink(ua::LoggerSinkFile::Instance());
	} else {
		ua::LoggerSinkStdout::Instance().showHeader(true);
		ua::logstream.sink(ua::LoggerSinkStdout::Instance());
	}

	ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m, level);
	ua::Logger::Instance().subSysHeaderSet(1, "ScanLib");
	loggingInitialized = true;
}

void configLogging(unsigned level) {
	configLogging(true, level);
}

/*
 * Could not use C++ streams for Release version of DLL.
 */
void saveResults(string & msg) {
	FILE * fh = fopen("scanlib.txt", "w");
	UA_ASSERT_NOT_NULL(fh);
	fprintf(fh, "%s", msg.c_str());
	fclose(fh);
}

int slIsTwainAvailable() {
	configLogging(3);
	UA_DOUT(1, 3, "slIsTwainAvailable:");

#ifdef WIN32
	ImageGrabber ig;
	if (ig.twainAvailable()) {
		return SC_SUCCESS;
	}
#endif
	return SC_TWAIN_UAVAIL;
}

int slSelectSourceAsDefault() {
	configLogging(3);
	UA_DOUT(1, 3, "slSelectSourceAsDefault:");

#ifdef WIN32
	ImageGrabber ig;
	if (ig.selectSourceAsDefault()) {
		return SC_SUCCESS;
	}
#endif
	return SC_FAIL;
}

int slScanImage(unsigned verbose, unsigned dpi, int brightness, int contrast,
		double left, double top, double right, double bottom, char * filename) {
	configLogging(verbose);
	UA_DOUT(1, 3, "slScanImage: dpi/" << dpi
			<< " brightness/" << brightness
			<< " contrast/" << contrast
			<< " left/" << left
			<< " top/"<< top
			<< " right/"<< right
			<< " bottom/"<< bottom);

#ifdef WIN32
	if (filename == NULL) {
		return SC_FAIL;
	}

	ImageGrabber ig;
	HANDLE h = ig.acquireImage(dpi, brightness, contrast, left, top, right,
			bottom);
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

int slDecodeCommon(unsigned plateNum, Dib & dib, unsigned scanGap,
		unsigned squareDev, unsigned edgeThresh) {
	int processImage = 0;
	Decoder decoder(scanGap, squareDev, edgeThresh);
	string msg;

	if (processImage) {
		Dib processedDib(dib);
		processedDib.expandColours(100, 200);
		processedDib.writeToFile("processed.bmp");

		if (!decoder.processImageRegions(plateNum, processedDib, msg)) {
			return SC_INVALID_IMAGE;
		}
	} else {
		if (!decoder.processImageRegions(plateNum, dib, msg)) {
			return SC_INVALID_IMAGE;
		}

	}

	saveResults(msg);

	Dib markedDib(dib);
	decoder.imageShowBarcodes(markedDib);

	markedDib.writeToFile("decoded.bmp");

	Util::getTime(endtime);
	Util::difftiime(starttime, endtime, timediff);
	UA_DOUT(1, 1, "slDecodeCommon: time taken: " << timediff);

	return SC_SUCCESS;
}

int slDecodePlate(unsigned verbose, unsigned dpi, int brightness, int contrast,
		unsigned plateNum, double left, double top, double right,
		double bottom, unsigned scanGap, unsigned squareDev,
		unsigned edgeThresh) {
	configLogging(verbose);
	UA_DOUT(1, 3, "slDecodePlate: dpi/" << dpi
			<< " brightness/" << brightness
			<< " contrast/" << contrast
			<< " plateNum/" << plateNum
			<< " left/" << left
			<< " top/"<< top
			<< " right/"<< right
			<< " bottom/"<< bottom
			<< " scanGap/" << scanGap
			<< " squareDev/" << squareDev
			<< " edgeThresh/" << edgeThresh);

#ifdef WIN32
	if (dpi < 0 || dpi > 2400) {
		return SC_INVALID_DPI;
	}

	if ((plateNum < MIN_PLATE_NUM) || (plateNum > MAX_PLATE_NUM)) {
		return SC_INVALID_PLATE_NUM;
	}

	ScFrame f;
	ImageGrabber ig;
	HANDLE h;
	int result;
	Dib dib;
	Util::getTime(starttime);

	h = ig.acquireImage(dpi, brightness, contrast, f.x0, f.y0, f.x1, f.y1);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not acquire plate image: " << plateNum);
		return SC_FAIL;
	}

	dib.readFromHandle(h);
	dib.writeToFile("scanned.bmp");
	result = slDecodeCommon(plateNum, dib, scanGap, squareDev, edgeThresh);
	ig.freeImage(h);
	return result;
#else
	return SC_FAIL;
#endif
}

int slDecodeImage(unsigned verbose, unsigned plateNum, char * filename,
		unsigned scanGap, unsigned squareDev, unsigned edgeThresh) {
	configLogging(verbose);
	UA_DOUT(1, 3, "slDecodeImage: plateNum/" << plateNum
			<< " filename/"<< filename
			<< " scanGap/" << scanGap
			<< " squareDev/" << squareDev
			<< " edgeThresh/" << edgeThresh);

	if ((plateNum < MIN_PLATE_NUM) || (plateNum > MAX_PLATE_NUM)) {
		return SC_INVALID_PLATE_NUM;
	}

	if (filename == NULL) {
		return SC_FAIL;
	}

	Util::getTime(starttime);

	Dib dib;

	dib.readFromFile(filename);
	return slDecodeCommon(plateNum, dib, scanGap, squareDev, edgeThresh);
}
