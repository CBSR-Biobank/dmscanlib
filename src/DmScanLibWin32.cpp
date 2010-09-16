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

#include <ctime>
#include <cstdlib>
#include <math.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

using namespace std;

int slIsTwainAvailable() {
    ImageGrabber ig;
    if (ig.twainAvailable()) {
        return SC_SUCCESS;
    }
    return SC_TWAIN_UAVAIL;
}

int slSelectSourceAsDefault() {
    ImageGrabber ig;
    if (ig.selectSourceAsDefault()) {
        return SC_SUCCESS;
    }
    return SC_FAIL;
}

/*
 * Please note that the 32nd bit should be ignored. 
 */
int slGetScannerCapability() {
    ImageGrabber ig;
    return ig.getScannerCapability();
}

int isValidDpi(int dpi) {
    ImageGrabber ig;
    int dpiCap = ig.getScannerCapability();
    return ((dpiCap & CAP_DPI_300) && dpi == 300)
    || ((dpiCap & CAP_DPI_400) && dpi == 400)
    || ((dpiCap & CAP_DPI_600) && dpi == 600);
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
    if (dib.getDpi() != dpi) {
        return SC_INCORRECT_DPI_SCANNED;
    }
    dib.writeToFile(filename);
    ig.freeImage(h);
    return SC_SUCCESS;
}

int slDecodePlate(unsigned verbose, unsigned dpi, int brightness, int contrast,
        unsigned plateNum, double left, double top, double right,
        double bottom, double scanGap, unsigned squareDev, unsigned edgeThresh,
        unsigned corrections, double cellDistance, double gapX, double gapY,
        unsigned profileA, unsigned profileB, unsigned profileC,
        unsigned isVertical) {
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
            << " cellDistance/" << cellDistance
            << " gapX/" << gapX
            << " gapY/" << gapY
            << " isVertical/" << isVertical);

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
            cellDistance);

    h = ig.acquireImage(dpi, brightness, contrast, left, top, right,
            bottom);
    if (h == NULL) {
        UA_DOUT(1, 1, "could not acquire plate image: " << plateNum);
        return ig.getErrorCode();
    }

    dib.readFromHandle(h);
    if (dib.getDpi() != dpi) {
        return SC_INCORRECT_DPI_SCANNED;
    }

    dib.writeToFile("scanned.bmp");
    result = slDecodeCommon(plateNum, dib, scanGap, squareDev, edgeThresh,
    		corrections, cellDistance, gapX, gapY, profileA, profileB, profileC,
    		isVertical, "decode.bmp");

    ig.freeImage(h);
    return result;
}
