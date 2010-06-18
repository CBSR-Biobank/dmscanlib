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
#include <sstream>
#include <string>

//XXX KERNELTEST
#include <ctime>
#include <cstdlib>
#include <math.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

using namespace std;

const char * INI_FILE_NAME = "scanlib.ini";

slTime starttime; // for debugging
slTime endtime;
slTime timediff;

static bool loggingInitialized = false;

void configLogging(unsigned level, bool useFile = true) {
	if (!loggingInitialized) {
		if (useFile) {
			ua::LoggerSinkFile::Instance().setFile("scanlib.log");
			ua::LoggerSinkFile::Instance().showHeader(true);
			ua::logstream.sink(ua::LoggerSinkFile::Instance());
		} else {
			ua::LoggerSinkStdout::Instance().showHeader(true);
			ua::logstream.sink(ua::LoggerSinkStdout::Instance());
		}
		ua::Logger::Instance().subSysHeaderSet(1, "ScanLib");
		loggingInitialized = true;
	}

	ua::Logger::Instance().levelSet(ua::LoggerImpl::allSubSys_m, level);
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
 Please note that the 32nd bit should be ignored.
 */
int slGetScannerCapability() {
#ifdef WIN32
	ImageGrabber ig;
	return ig.getScannerCapability();
#endif
	return 0xFF; //supports WIA and DPI: 300,400,600
}

int isValidDpi(int dpi) {
#ifdef WIN32
	ImageGrabber ig;
	int dpiCap = ig.getScannerCapability();
	return ((dpiCap & CAP_DPI_300) && dpi == 300) || ((dpiCap & CAP_DPI_400)
			&& dpi == 400) || ((dpiCap & CAP_DPI_600) && dpi == 600);
#endif
	return 1;
}

void formatCellMessages(unsigned plateNum, vector<vector<string> > & cells,
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
		const char * filename) {
	configLogging(verbose);
	UA_DOUT(1, 3, "slScanImage: dpi/" << dpi
			<< " brightness/" << brightness
			<< " contrast/" << contrast
			<< " left/" << left
			<< " top/"<< top
			<< " right/"<< right
			<< " bottom/"<< bottom);

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
		const char * markedDibFilename, vector<vector<string> > & cellsRef) {
	string msg;

	Dib * grayscaleDib = NULL;
	grayscaleDib = Dib::convertGrayscale(dib);
	UA_ASSERT_NOT_NULL(grayscaleDib);

	grayscaleDib->tpPresetFilter();

	UA_DEBUG(
			grayscaleDib->writeToFile("filtered.bmp");
			//Dib dibBlobs(*grayscaleDib);
			//detectBlobs(*grayscaleDib,dibBlobs);
			//dibBlobs.writeToFile("BLOBS.bmp");
	);

	Decoder::ProcessResult result = decoder.processImageRegions(plateNum,
			*grayscaleDib, cellsRef);

	if (result == Decoder::OK) {
		decoder.imageShowBarcodes(*grayscaleDib);
		grayscaleDib->writeToFile(markedDibFilename);
	}
	delete grayscaleDib;

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
	UA_DOUT(1, 1, "slDecodeCommon: time taken: " << timediff);
	return SC_SUCCESS;
}

int slDecodePlate(unsigned verbose, unsigned dpi, int brightness, int contrast,
		unsigned plateNum, double left, double top, double right,
		double bottom, double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance) {
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
			<< " edgeThresh/" << edgeThresh
			<< " corrections/" << corrections
			<< " cellDistance/" << cellDistance);

#ifdef WIN32
	ImageGrabber ig;

	if ((plateNum < MIN_PLATE_NUM) || (plateNum > MAX_PLATE_NUM)) {
		return SC_INVALID_PLATE_NUM;
	}

	HANDLE h;
	int result;
	Dib dib;
	vector<vector<string> > cells;
	Util::getTime(starttime);
	Decoder decoder(scanGap, squareDev, edgeThresh, corrections, cellDistance);

	h = ig.acquireImage(dpi, brightness, contrast, left, top, right, bottom);
	if (h == NULL) {
		UA_DOUT(1, 1, "could not acquire plate image: " << plateNum);
		return ig.getErrorCode();
	}

	dib.readFromHandle(h);
	dib.writeToFile("scanned.bmp");

	if (dib.getDpi() != dpi)
		result = SC_INCORRECT_DPI_SCANNED;
	else
		result = slDecodeCommon(plateNum, dib, decoder, "decode.bmp", cells);

	if (result == SC_SUCCESS) {
		string msg;
		formatCellMessages(plateNum, cells, msg);
		saveResults(msg);
	}

	ig.freeImage(h);
	return result;
#else
	return SC_FAIL;
#endif
}

int slDecodePlateMultipleDpi(unsigned verbose, unsigned dpi1, unsigned dpi2,
		unsigned dpi3, int brightness, int contrast, unsigned plateNum,
		double left, double top, double right, double bottom, double scanGap,
		unsigned squareDev, unsigned edgeThresh, unsigned corrections,
		double cellDistance) {
	configLogging(verbose);
	UA_DOUT(1, 3, "slDecodePlateMultipleDpi: dpi1/" << dpi1
			<< " dpi2/" << dpi2
			<< " dpi3/" << dpi3
			<< " brightness/" << brightness
			<< " contrast/" << contrast
			<< " plateNum/" << plateNum
			<< " left/" << left
			<< " top/"<< top
			<< " right/"<< right
			<< " bottom/"<< bottom
			<< " scanGap/" << scanGap
			<< " squareDev/" << squareDev
			<< " edgeThresh/" << edgeThresh
			<< " corrections/" << corrections
			<< " cellDistance/" << cellDistance);

#ifdef WIN32
	ImageGrabber ig;

	Util::getTime(starttime);
	unsigned dpis[] = { dpi1, dpi2, dpi3 };

	std::ostringstream filename;

	HANDLE h;
	int result = SC_FAIL;
	Dib dib;
	vector<vector<string> > cells;
	vector<vector<string> > newCells;

	for (unsigned i = 0; i < 3; ++i) {
		if (dpis[i] == 0)
			continue;

		Decoder * decoder = new Decoder(scanGap, squareDev, edgeThresh,
				corrections, cellDistance);
		UA_ASSERT_NOT_NULL(decoder);

		h = ig.acquireImage(dpis[i], brightness, contrast, left, top, right,
				bottom);
		if (h == NULL) {
			UA_DOUT(1, 1, "could not acquire plate image: " << plateNum);
			return ig.getErrorCode();
		}

		dib.readFromHandle(h);
		filename.str("");
		filename << "scanned" << i + 1 << ".bmp";
		dib.writeToFile(filename.str().c_str());
		filename.str("");
		filename << "decoded" << i + 1 << ".bmp";

		if (dib.getDpi() != dpis[i])
			result = SC_INCORRECT_DPI_SCANNED;
		else
			result = slDecodeCommon(plateNum, dib, *decoder,
					filename.str().c_str(), newCells);

		if (result != SC_SUCCESS) {
			return result;
		}

		if (cells.size() < newCells.size()) {
			cells = newCells;
		} else {
			for (unsigned row = 0, numRows = cells.size(); row < numRows; ++row) {
				for (unsigned col = 0, numCols = newCells[row].size(); col
						< numCols; ++col) {
					if ((cells[row][col].length() > 0)
							&& (newCells[row][col].length() > 0)
							&& (cells[row][col] != newCells[row][col])) {
						UA_WARN("current cell and new cell do not match: row/"
								<< row << " col/" << col << " current/" << cells[row][col]
								<< " new/" << newCells[row][col]);
					} else {
						cells[row][col] = newCells[row][col];
					}
				}
			}
		}
		ig.freeImage(h);
		delete decoder;
	}

	if (result == SC_SUCCESS) {
		string msg;
		formatCellMessages(plateNum, cells, msg);
		saveResults(msg);
	}

	Util::getTime(endtime);
	Util::difftiime(starttime, endtime, timediff);
	UA_DOUT(1, 1, "slDecodePlateMultipleDpi: time taken: " << timediff);

	return result;
#else
	return SC_FAIL;
#endif

}

int slDecodeImage(unsigned verbose, unsigned plateNum, const char * filename,
		double scanGap, unsigned squareDev, unsigned edgeThresh,
		unsigned corrections, double cellDistance) {
	configLogging(verbose);
	UA_DOUT(1, 3, "slDecodeImage: plateNum/" << plateNum
			<< " filename/"<< filename
			<< " scanGap/" << scanGap
			<< " squareDev/" << squareDev
			<< " edgeThresh/" << edgeThresh
			<< " corrections/" << corrections
			<< " cellDistance/" << cellDistance);

	if ((plateNum < MIN_PLATE_NUM) || (plateNum > MAX_PLATE_NUM)) {
		return SC_INVALID_PLATE_NUM;
	}

	if (filename == NULL) {
		return SC_FAIL;
	}

	Util::getTime(starttime);

	Dib dib;
	vector<vector<string> > cells;
	Decoder decoder(scanGap, squareDev, edgeThresh, corrections, cellDistance);

	dib.readFromFile(filename);

	int result = slDecodeCommon(plateNum, dib, decoder, "decode.bmp", cells);

	if (result == SC_SUCCESS) {
		string msg;
		formatCellMessages(plateNum, cells, msg);
		saveResults(msg);
	}
	return result;
}
