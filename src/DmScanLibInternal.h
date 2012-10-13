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

#if ! defined _VISUALC_
#   include <tr1/memory>
#endif

class Dib;
class Decoder;
class ImgScanner;
class WellDecoder;
class DecodeOptions;
class DecodedWell;

using namespace std;

void getResultCodeMsg(int resultCode, string & message);
jobject createScanResultObject(JNIEnv * env, int resultCode, int value);
jobject createDecodedResultObject(JNIEnv * env, int resultCode,
        std::vector<std::tr1::shared_ptr<DecodedWell> > * wells);

class DmScanLib {
public:
    DmScanLib(unsigned loggingLevel, bool logToFile = true);
    virtual ~DmScanLib();

     int isTwainAvailable();

     int selectSourceAsDefault();
     int getScannerCapability();
     int scanImage(unsigned dpi, int brightness, int contrast,
                   double left, double top, double right, double bottom,
                   const string & filename);
     int scanFlatbed(unsigned dpi, int brightness, int contrast,
                     const string & filename);
     int scanAndDecode(unsigned dpi, int brightness, int contrast,
                            double left, double top, double right,
                            double bottom, double scanGap,
                            unsigned squareDev, unsigned edgeThresh,
                            unsigned corrections, double cellDistance);
    int decodeImage(const char * filename, DecodeOptions & decodeOptions,
    		vector<std::tr1::shared_ptr<WellRectangle<double>  > > & wellRects);

    void configLogging(unsigned level, bool useFile = true);

    void setTextFileOutputEnable(bool enable) {
        textFileOutputEnable = enable;
    }

    void setStdoutOutputEnable(bool enable) {
        stdoutOutputEnable = enable;
    }

    //std::vector<std::tr1::shared_ptr<WellDecoder> > & getDecodedCells();

protected:
    void saveResults(string & msg);

    void formatCellMessages(unsigned plateNum, string & msg);

    int isValidDpi(int dpi);

    int decodeCommon(const string &markedDibFilename);

    void applyFilters();

    static const string LIBRARY_NAME;

    std::tr1::shared_ptr<Dib> image;
    std::tr1::shared_ptr<ImgScanner> imgScanner;
    std::tr1::shared_ptr<Decoder> decoder;

    double scanGap;
    unsigned squareDev;
    unsigned edgeThresh;
    unsigned corrections;
    double cellDistance;

    slTime starttime; // for debugging
    slTime endtime;
    slTime timediff;

    bool stdoutOutputEnable;

    bool textFileOutputEnable;

    static bool loggingInitialized;

};

#endif /* __INC_SCANLIB_INTERNAL_H */
