/*******************************************************************************
 * Canadian Biosample Repository
 *
 * DmScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 * Dmscanlib is a software library and standalone application that scans
 * and decodes libdmtx compatible test-tubes. It is currently designed
 * to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 *  Copyright (C) 2010 Canadian Biosample Repository
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include "DmScanLib.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Decoder.h"
#include "Dib.h"
#include "TimeUtil.h"
#include "BarcodeInfo.h"
#include "PalletGrid.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <ctime>
#include <cstdlib>
#include <math.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

using namespace std;

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

void formatCellMessages(unsigned plateNum, Decoder & decoder, string & msg) {
    ostringstream out;
    out << "#Plate,Row,Col,Barcode" << endl;

    for (unsigned row = 0; row < PalletGrid::MAX_ROWS; ++row) {
        for (unsigned col = 0; col < PalletGrid::MAX_COLS; ++col) {
            const char * msg = decoder.getBarcode(row, col);
            if (msg == NULL) continue;
            out << plateNum << "," << static_cast<char> ('A' + row) << ","
                    << (col + 1) << "," << msg << endl;
        }
    }
    msg = out.str();
}

int slDecodeCommon(unsigned plateNum, Dib & dib, double scanGap,
        unsigned squareDev, unsigned edgeThresh, unsigned corrections,
        double cellDistance, double gapX, double gapY, unsigned profileA,
        unsigned profileB, unsigned profileC, unsigned isVertical,
        const char *markedDibFilename) {

    bool metrical = false;
    Dib *filteredDib;
    Decoder::ProcessResult result;

    PalletGrid::Orientation orientation =
            (isVertical ? PalletGrid::ORIENTATION_VERTICAL
                    : PalletGrid::ORIENTATION_HORIZONTAL);

    unsigned dpi = dib.getDpi();
    unsigned gapXpixels = static_cast<unsigned>(dpi * gapX);
    unsigned gapYpixels = static_cast<unsigned>(dpi * gapY);

    vector<unsigned> profileWords;
    profileWords.push_back(profileA);
    profileWords.push_back(profileB);
    profileWords.push_back(profileC);

    PalletGrid * palletGrid = new PalletGrid(orientation, dib.getWidth(),
            dib.getHeight(), gapXpixels, gapYpixels, profileWords);

    if (!palletGrid->isImageValid()) {
        return SC_INVALID_IMAGE;
    }

    Decoder decoder(scanGap, squareDev, edgeThresh, corrections, cellDistance,
            palletGrid);

    UA_DOUT(1, 5, "DecodeCommon: metrical mode: " << metrical);

    /*--- apply filters ---*/
    filteredDib = Dib::convertGrayscale(dib);
    UA_ASSERT_NOT_NULL(filteredDib);

    filteredDib->tpPresetFilter();
    UA_DEBUG(
            filteredDib->writeToFile("filtered.bmp")
    );

    /*--- obtain barcodes ---*/
    result = decoder.processImageRegions(filteredDib);

    delete filteredDib;

    decoder.imageShowBarcodes(dib, 0);
    if (result == Decoder::OK)
        dib.writeToFile(markedDibFilename);
    else
        dib.writeToFile("decode.partial.bmp");

    switch (result) {
    case Decoder::IMG_INVALID:
        return SC_INVALID_IMAGE;

    default:
        ; // do nothing
    }

    // only get here if decoder returned Decoder::OK
    string msg;
    formatCellMessages(plateNum, decoder, msg);
    saveResults(msg);

    Util::getTime(endtime);
    Util::difftiime(starttime, endtime, timediff);
    UA_DOUT(1, 1, "slDecodeCommonCv: time taken: " << timediff);
    return SC_SUCCESS;
}

int slDecodeImage(unsigned verbose, unsigned plateNum, const char *filename,
        double scanGap, unsigned squareDev, unsigned edgeThresh,
        unsigned corrections, double cellDistance, double gapX, double gapY,
        unsigned profileA, unsigned profileB, unsigned profileC,
        unsigned isVertical) {
    configLogging(verbose);
    UA_DOUT(1, 3, "slDecodeImage: plateNum/" << plateNum
            << " filename/" << filename
            << " scanGap/" << scanGap
            << " squareDev/" << squareDev
            << " edgeThresh/" << edgeThresh
            << " corrections/" << corrections
            << " cellDistance/" << cellDistance
            << " gapX/" << gapX
            << " gapY/" << gapY
            << " isVertical/" << isVertical);

    if ((plateNum < MIN_PLATE_NUM) || (plateNum > MAX_PLATE_NUM)) {
        return SC_INVALID_PLATE_NUM;
    }

    if (filename == NULL) {
        return SC_FAIL;
    }

    Util::getTime(starttime);

    Dib dib;
    dib.readFromFile(filename);

    int result = slDecodeCommon(plateNum, dib, scanGap, squareDev, edgeThresh,
            corrections, cellDistance, gapX, gapY, profileA, profileB, profileC,
            isVertical, "decode.bmp");
    return result;
}

