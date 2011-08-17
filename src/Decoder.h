#ifndef DECODER_H_
#define DECODER_H_

/*
 * Dmscanlib is a software library and standalone application that scans
 * and decodes libdmtx compatible test-tubes. It is currently designed
 * to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 * Copyright (C) 2010 Canadian Biosample Repository
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "dmtx.h"
#include "cv.h"
#include "IplContainer.h"
#include "PalletGrid.h"

#include <string>
#include <OpenThreads/Mutex>

#ifdef _VISUALC_
#   include <functional>
#else
#   include <tr1/functional>
#endif

#ifdef WIN32
#include <windows.h>
#endif

using namespace std;

class Dib;
struct RgbQuad;
class BinRegion;

class Decoder {
public:
    typedef std::tr1::function<
                    void(std::string & decodedMsg, CvPoint(&corners)[4])> DecodeCallback;

    Decoder(unsigned dpi, double scanGap, unsigned squareDev, unsigned edgeThresh,
            unsigned corrections, double cellDistance);
    virtual ~Decoder();

    void decodeImage(std::tr1::weak_ptr<const Dib> dib, DecodeCallback callback);

    void writeDiagnosticImage(DmtxDecode *dec, string & id);

private:

    static const unsigned PALLET_ROWS;
    static const unsigned PALLET_COLUMNS;
    static const double BARCODE_SIDE_LENGTH_INCHES;

    static DmtxImage * createDmtxImageFromDib(const Dib & dib);
    void getDecodeInfo(DmtxDecode *dec, DmtxRegion *reg,
    		DmtxMessage *msg, DecodeCallback callback);

    void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);

    unsigned dpi;
    double scanGap;
    unsigned squareDev;
    unsigned edgeThresh;
    unsigned corrections;
    double cellDistance;
    unsigned width;
    unsigned height;

    OpenThreads::Mutex addBarcodeMutex;
};

#endif /* DECODER_H_ */
