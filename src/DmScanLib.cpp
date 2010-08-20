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

// XXX KERNELTEST
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

struct Pt2d{
	int x;
	int y;
};

Pt2d rotate(Pt2d point, Pt2d pivot,float radians){
	
	Pt2d rotated;
	rotated.x = (int) (pivot.x + (point.x - pivot.x)*cos(radians) - (point.y - pivot.y)*sin(radians));
	rotated.y = (int) (pivot.y + (point.x - pivot.x)*sin(radians) + (point.y - pivot.y)*cos(radians));
	
	return rotated;
}
/*
Pt2d redCenter(Dib dib){

	Dib redBitmap(dib);

	float avgx=0,avgy=0,weightsum=0;

	RgbQuad color;
	for(int y=0,h=redBitmap.getHeight();y < h; y++){
		for(int x=0,w=redBitmap.getWidth();x < w; x++){
			redBitmap.getPixel(y,x,color);

			if(color.rgbRed < 40){
				color.rgbBlue = 0;
				color.rgbGreen = 0;
				color.rgbRed = 0;
			}
			else{
				if(  ((float)color.rgbGreen/(float)color.rgbRed) > 0.75){
					color.rgbBlue = 0;
					color.rgbGreen = 0;
					color.rgbRed = 0;
				}
			}
			float multiplier = (float)color.rgbRed*color.rgbRed*color.rgbRed;
			avgx += multiplier*x;
			avgy += multiplier*y;
			weightsum += multiplier;
			
			redBitmap.setPixel(x,y,color);
		}
	}
	avgx /= weightsum;
	avgy /= weightsum;


	Pt2d pos;
	pos.x = (int)avgx;
	pos.y = (int)avgy;

	return pos;
}

Pt2d greenCenter(Dib dib){

	Dib greenBitmap(dib);

	float avgx=0,avgy=0,weightsum=0;

	RgbQuad color;
	for(int y=0,h=greenBitmap.getHeight();y < h; y++){
		for(int x=0,w=greenBitmap.getWidth();x < w; x++){
			greenBitmap.getPixel(y,x,color);

			if(color.rgbGreen < 40){
				color.rgbBlue = 0;
				color.rgbGreen = 0;
				color.rgbRed = 0;
			}
			else{
				if(  ((float)color.rgbRed/(float)color.rgbGreen) > 0.75){
					color.rgbBlue = 0;
					color.rgbGreen = 0;
					color.rgbRed = 0;
				}
			}
			float multiplier = (float)color.rgbGreen*color.rgbGreen*color.rgbGreen;
			avgx += multiplier*x;
			avgy += multiplier*y;
			weightsum += multiplier;
			
			greenBitmap.setPixel(x,y,color);
		}
	}
	avgx /= weightsum;
	avgy /= weightsum;


	Pt2d pos;
	pos.x = (int)avgx;
	pos.y = (int)avgy;

	return pos;
}
*/

int slDecodeCommon(unsigned plateNum, Dib & dib, Decoder & decoder,
		const char *markedDibFilename) {

	bool metrical = false;
	Dib *filteredDib;
	Decoder::ProcessResult result;

	UA_DOUT(1, 2, "Running slDecodeCommonCv");

	UA_DOUT(1, 4, "DecodeCommon: metrical mode: " << metrical);

	/*
	UA_DOUT(1, 1, "===========DEBUG==============");
	

	RgbQuad color;
	
	Pt2d redPos = redCenter(dib);
	Pt2d greenPos = greenCenter(dib);

	
	for(int y=0,h=dib.getHeight();y < h; y++){
		for(int x=0,w=dib.getWidth();x < w; x++){
			dib.getPixel(y,x,color);

			if(color.rgbGreen + color.rgbBlue + color.rgbRed < 500){
				color.rgbBlue = 0;
				color.rgbGreen = 0;
				color.rgbRed = 0;
			}

			dib.setPixel(x,y,color);
		}
	}

	RgbQuad center;
	RgbQuad adjacent[4];
	RgbQuad buffer;
	float sum;

	double contrastRatio = 0;

	for(int y=1,h=dib.getHeight();y < h-1; y++){
		for(int x=1,w=dib.getWidth();x < w-1; x++){
			
			dib.getPixel(y,x,center);
			dib.getPixel(y-1,x,adjacent[0]);
			dib.getPixel(y,x-1,adjacent[1]);
			dib.getPixel(y,x+1,adjacent[2]);
			dib.getPixel(y+1,x,adjacent[3]);


			sum = 0;
			for(int i=0; i < 4; i++)
				sum += (float)(adjacent[i]).rgbBlue;
			buffer.rgbBlue = (unsigned char)(sum/4.0);

			sum = 0;
			for(int i=0; i < 4; i++)
				sum += (float)(adjacent[i]).rgbGreen;
			buffer.rgbGreen = (unsigned char)(sum/4.0);
			
			sum = 0;
			for(int i=0; i < 4; i++)
				sum += (float)(adjacent[i]).rgbRed;
			buffer.rgbRed = (unsigned char)(sum/4.0);

			contrastRatio += (pow(abs((float)buffer.rgbBlue - (float)center.rgbBlue),2) +
							pow(abs((float)buffer.rgbGreen - (float)center.rgbGreen),2) +
							pow(abs((float)buffer.rgbRed - (float)center.rgbRed),2))/3.0;
		}
	}

	contrastRatio /= (dib.getHeight()-2)*(dib.getWidth()-2);

	UA_DOUT(1, 1, "Contrast Ratio: " << contrastRatio );
	

	color.set(255,255,255);

	float angle = (float)(atan( ((float)(greenPos.y - redPos.y))/((float)(greenPos.x - redPos.x))));
	angle = -0.15;
	//expected [right] angle 33.69 degrees (0.588002604 rad) 


	UA_DOUT(1, 1, "Angle: " << angle );


	int ox,oy;
	ox = 110;
	oy = 25;

	int gapx,gapy;
	gapx = 26;
	gapy = 26;

	int startx,starty;
	startx = redPos.x + ox;
	starty = (dib.getHeight()-(redPos.y - oy) );


	float dist = sqrt((float)(redPos.y - greenPos.y)*(redPos.y - greenPos.y) + (redPos.x - greenPos.x)*(redPos.x  - greenPos.x)) - 220;
	int scalex,scaley;
	scalex = 80;
	scaley = 80;
	for(int y =0; y < 8; y++){
		for(int x =0; x < 12; x++){

			int x0 = startx + x*scalex + x*gapx;
			int y0 = starty + y*scaley + y*gapy;

			Pt2d start;
			Pt2d pt;
			Pt2d rotated;

			start.x = startx;
			start.y = starty;
			pt.x = x0;
			pt.y = y0;
			rotated = rotate(pt,start,angle);

			dib.rectangleRotated(rotated.x,rotated.y,scalex,scaley,color,angle);
		}
	}


	dib.line(redPos.x,dib.getHeight()-redPos.y,greenPos.x,dib.getHeight()-greenPos.y,color);

	dib.writeToFile("ready.bmp");

	exit(0);

	*/
	
	


	/*--- apply filters ---*/
	filteredDib = Dib::convertGrayscale(dib);
	UA_ASSERT_NOT_NULL(filteredDib);
	filteredDib->tpPresetFilter();

	UA_DEBUG(filteredDib->writeToFile("filtered.bmp"));

	result = decoder.processImageRegions(filteredDib, metrical);

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
		unsigned profileA,unsigned profileB, unsigned profileC) {
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
			<< " cellDistance/" << cellDistance);

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
			cellDistance,gapX,gapY,profileA,profileB,profileC);

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
		unsigned profileA,unsigned profileB, unsigned profileC) {
	configLogging(verbose);
	UA_DOUT(1, 3, "slDecodeImage: plateNum/" << plateNum
			<< " filename/" << filename
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
	Decoder decoder(scanGap, squareDev, edgeThresh, corrections, cellDistance,
		gapX,gapY,profileA,profileB,profileC);

	dib.readFromFile(filename);

	int result = slDecodeCommon(plateNum, dib, decoder, "decode.bmp");

	if (result == SC_SUCCESS) {
		string msg;
		formatCellMessages(plateNum, decoder.getCells(), msg);
		saveResults(msg);
	}
	return result;
}
