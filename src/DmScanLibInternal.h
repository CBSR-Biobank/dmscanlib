#ifndef __INC_SCANLIB_INTERNAL_H
#define __INC_SCANLIB_INTERNAL_H

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

#include "TimeUtil.h"

#include <string>
#include <memory>
#include <vector>

class Dib;
class Decoder;
class ImgScanner;
class BarcodeInfo;

using namespace std;

class DmScanLib {
public:
	DmScanLib();
	virtual ~DmScanLib();

	virtual int isTwainAvailable();

	virtual int selectSourceAsDefault();
	virtual int getScannerCapability();
	virtual int scanImage(unsigned verbose, unsigned dpi, int brightness,
			int contrast, double left, double top, double right, double bottom,
			const char * filename);
	virtual int scanFlatbed(unsigned verbose, unsigned dpi, int brightness,
			int contrast, const char * filename);
	virtual int decodePlate(unsigned verbose, unsigned dpi, int brightness,
			int contrast, unsigned plateNum, double left, double top,
			double right, double bottom, double scanGap, unsigned squareDev,
			unsigned edgeThresh, unsigned corrections, double cellDistance,
			double gapX, double gapY, unsigned profileA, unsigned profileB,
			unsigned profileC, unsigned isHoriztonal);
	virtual int decodeImage(unsigned verbose, unsigned plateNum,
			const char * filename, double scanGap, unsigned squareDev,
			unsigned edgeThresh, unsigned corrections, double cellDistance,
			double gapX, double gapY, unsigned profileA, unsigned profileB,
			unsigned profileC, unsigned isHoriztonal);

	void configLogging(unsigned level, bool useFile = true);

	void setTextFileOutputEnable(bool enable) {
		textFileOutputEnable = enable;
	}

protected:
	void saveResults(string & msg);

	vector<BarcodeInfo *> & getBarcodes();

	void formatCellMessages(unsigned plateNum, string & msg);

	int isValidDpi(int dpi);

	int decodeCommon(unsigned plateNum, Dib & dib, double scanGap,
	        unsigned squareDev, unsigned edgeThresh, unsigned corrections,
	        double cellDistance, double gapX, double gapY, unsigned profileA,
	        unsigned profileB, unsigned profileC, unsigned isVertical,
	        const char *markedDibFilename);

	auto_ptr<ImgScanner> imgScanner;
	auto_ptr<Decoder> decoder;

	slTime starttime; // for debugging
	slTime endtime;
	slTime timediff;

	bool textFileOutputEnable;

	static bool loggingInitialized;

};

#endif /* __INC_SCANLIB_INTERNAL_H */
