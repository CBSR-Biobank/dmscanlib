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

#include "WellRectangle.h"
#include "utils/TimeUtil.h"

#include <string>
#include <memory>
#include <vector>
#include <jni.h>

/**
 * Return codes used by the DLL API.
 */
const int SC_SUCCESS = 0;
const int SC_FAIL = -1;
const int SC_TWAIN_UNAVAIL = -2;
const int SC_INVALID_DPI = -3;
const int SC_INVALID_PLATE_NUM = -4;
const int SC_INVALID_VALUE = -5;
const int SC_INVALID_IMAGE = -6;
const int SC_INCORRECT_DPI_SCANNED = -9;

const unsigned CAP_IS_WIA = 0x01;
const unsigned CAP_DPI_300 = 0x02;
const unsigned CAP_DPI_400 = 0x04;
const unsigned CAP_DPI_600 = 0x08;
const unsigned CAP_IS_SCANNER = 0x10;


class Dib;
class Decoder;
class ImgScanner;
class WellDecoder;
class DecodeOptions;

using namespace std;

void getResultCodeMsg(int resultCode, string & message);
jobject createScanResultObject(JNIEnv * env, int resultCode, int value);
jobject createDecodedResultObject(JNIEnv * env, int resultCode,
        std::vector<unique_ptr<WellDecoder> > * wells);

class DmScanLib {
public:
	DmScanLib(unsigned loggingLevel, bool logToFile = true);
	virtual ~DmScanLib();

	int isTwainAvailable();

	int selectSourceAsDefault();
	int getScannerCapability();
	int scanImage(unsigned dpi, int brightness, int contrast,
			double left, double top, double right, double bottom,
			const char * filename);
	int scanFlatbed(unsigned dpi, int brightness, int contrast,
			const char * filename);
	int scanAndDecode(unsigned dpi, int brightness, int contrast,
			double left, double top, double right, double bottom,
			const DecodeOptions & decodeOptions,
			vector<unique_ptr<WellRectangle<double>  > > & wellRects);
	int decodeImageWells(const char * filename,
			const DecodeOptions & decodeOptions,
			vector<unique_ptr<WellRectangle<double>  > > & wellRects);

	static void configLogging(unsigned level, bool useFile = true);

	void setTextFileOutputEnable(bool enable) {
		textFileOutputEnable = enable;
	}

	void setStdoutOutputEnable(bool enable) {
		stdoutOutputEnable = enable;
	}

protected:
    void saveResults(string & msg);

    void formatCellMessages(unsigned plateNum, string & msg);

    int isValidDpi(int dpi);

    int decodeCommon(const Dib & image, const DecodeOptions & decodeOptions,
    		const string &markedDibFilename,
    	    vector<unique_ptr<WellRectangle<double>  > > & wellRects);

    static const string LIBRARY_NAME;

    slTime starttime; // for debugging
    slTime endtime;
    slTime timediff;

    std::unique_ptr<ImgScanner> imgScanner;

    bool stdoutOutputEnable;

    bool textFileOutputEnable;

    static bool loggingInitialized;

};

#endif /* __INC_SCANLIB_INTERNAL_H */
