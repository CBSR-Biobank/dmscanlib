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

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

#include "Decoder.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Dib.h"
#include "ProcessImageManager.h"
#include "PalletGrid.h"

#include <stdio.h>
#include <sstream>
#include <time.h>
#include <iostream>
#include <math.h>
#include <string>
#include <limits>
#include <vector>
#include <cmath>

#ifdef _VISUALC_
#   include <memory>
#else
#   include <tr1/memory>
#endif

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

using namespace std;

const double Decoder::BARCODE_SIDE_LENGTH_INCHES = 0.13;
const unsigned Decoder::PALLET_ROWS = 8;
const unsigned Decoder::PALLET_COLUMNS = 12;

Decoder::Decoder(unsigned _dpi, double g, unsigned s, unsigned t, unsigned c,
                 double dist)
                : dpi(_dpi), scanGap(g), squareDev(s), edgeThresh(t), corrections(
                                c), cellDistance(dist) {
    ua::Logger::Instance().subSysHeaderSet(3, "Decoder");
}

Decoder::~Decoder() {
}

void Decoder::decodeImage(std::tr1::weak_ptr<const Dib> dib,
                          DecodeCallback callback) {
    DmtxImage * image = createDmtxImageFromDib(*dib.lock());
    int minEdgeSize, maxEdgeSize;

    DmtxDecode *dec = dmtxDecodeCreate(image, 1);
    UA_ASSERT_NOT_NULL(dec);

    // slightly smaller than the new tube edge
    minEdgeSize = static_cast<int>(0.08 * dpi);

    // slightly bigger than the Nunc edge
maxEdgeSize    = static_cast<int>(0.18 * dpi);

dmtxDecodeSetProp    (dec, DmtxPropEdgeMin, minEdgeSize);
    dmtxDecodeSetProp(dec, DmtxPropEdgeMax, maxEdgeSize);
    dmtxDecodeSetProp(dec, DmtxPropSymbolSize, DmtxSymbolSquareAuto);
    dmtxDecodeSetProp(dec, DmtxPropScanGap,
                      static_cast<unsigned>(scanGap * dpi));dmtxDecodeSetProp
    (dec, DmtxPropSquareDevn, squareDev);
    dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, edgeThresh);

    bool msgFound = false;
    DmtxRegion * reg;
    while (1) {
        reg = dmtxRegionFindNext(dec, NULL);
        if (reg == NULL) {
            break;
        }

        DmtxMessage *msg = dmtxDecodeMatrixRegion(dec, reg, corrections);
        if (msg != NULL) {
            msgFound = true;
            callback(dec, reg, msg);
            dmtxMessageDestroy(&msg);
        }
        dmtxRegionDestroy(&reg);
    }

    // TODO: write image when in debug mode
//	if (debug) {
//		if (msgFound) {
//			writeDib("found");
//		} else {
//			writeDib("missed");
//		}
//		writeDiagnosticImage(dec);
//	}

    dmtxDecodeDestroy(&dec);
    dmtxImageDestroy(&image);
}

DmtxImage * Decoder::createDmtxImageFromDib(const Dib & dib) {
    int pack = DmtxPackCustom;
    unsigned padding = dib.getRowPadBytes();

    switch (dib.getBitsPerPixel()) {
    case 8:
        pack = DmtxPack8bppK;
        break;
    case 24:
        pack = DmtxPack24bppRGB;
        break;
    case 32:
        pack = DmtxPack32bppXRGB;
        break;
    }

    DmtxImage * image = dmtxImageCreate(dib.getPixelBuffer(), dib.getWidth(),
                                        dib.getHeight(), pack);

    //set the properties (pad bytes, flip)
    dmtxImageSetProp(image, DmtxPropRowPadBytes, padding);
    dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
    return image;
}

void Decoder::showStats(DmtxDecode * dec, DmtxRegion * reg, DmtxMessage * msg) {
    int height;
    int dataWordLength;
    int rotateInt;
    double rotate;
    DmtxVector2 p00, p10, p11, p01;

    height = dmtxDecodeGetProp(dec, DmtxPropHeight);

    p00.X = p00.Y = p10.Y = p01.X = 0.0;
    p10.X = p01.Y = p11.X = p11.Y = 1.0;
    dmtxMatrix3VMultiplyBy(&p00, reg->fit2raw);
    dmtxMatrix3VMultiplyBy(&p10, reg->fit2raw);
    dmtxMatrix3VMultiplyBy(&p11, reg->fit2raw);
    dmtxMatrix3VMultiplyBy(&p01, reg->fit2raw);

    dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords,
                                            reg->sizeIdx);

    rotate = (2 * M_PI)
                    + (atan2(reg->fit2raw[0][1], reg->fit2raw[1][1])
                                    - atan2(reg->fit2raw[1][0],
                                            reg->fit2raw[0][0])) / 2.0;

    rotateInt = (int) (rotate * 180 / M_PI + 0.5);
    if (rotateInt >= 360) rotateInt -= 360;

    fprintf(stdout, "--------------------------------------------------\n");
    fprintf(stdout, "       Matrix Size: %d x %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx));
    fprintf(stdout, "    Data Codewords: %d (capacity %d)\n",
            dataWordLength - msg->padCount, dataWordLength);
    fprintf(stdout,
            "   Error Codewords: %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords,
                                   reg->sizeIdx));
    fprintf(stdout, "      Data Regions: %d x %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, reg->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, reg->sizeIdx));
    fprintf(stdout,
            "Interleaved Blocks: %d\n",
            dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks,
                                   reg->sizeIdx));
    fprintf(stdout, "    Rotation Angle: %d\n", rotateInt);
    fprintf(stdout, "          Corner 0: (%0.1f, %0.1f)\n", p00.X,
            height - 1 - p00.Y);
    fprintf(stdout, "          Corner 1: (%0.1f, %0.1f)\n", p10.X,
            height - 1 - p10.Y);
    fprintf(stdout, "          Corner 2: (%0.1f, %0.1f)\n", p11.X,
            height - 1 - p11.Y);
    fprintf(stdout, "          Corner 3: (%0.1f, %0.1f)\n", p01.X,
            height - 1 - p01.Y);
    fprintf(stdout, "--------------------------------------------------\n");
}

void Decoder::writeDiagnosticImage(DmtxDecode *dec, string & id) {
    int totalBytes, headerBytes;
    int bytesWritten;
    unsigned char *pnm;
    FILE *fp;

    ostringstream fname;
    fname << "diagnostic-" << id << ".pnm";

    fp = fopen(fname.str().c_str(), "wb");
    UA_ASSERT_NOT_NULL(fp);

    pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
    UA_ASSERT_NOT_NULL(pnm);

    bytesWritten = fwrite(pnm, sizeof(unsigned char), totalBytes, fp);
    UA_ASSERT(bytesWritten == totalBytes);

    free(pnm);
    fclose(fp);
}
