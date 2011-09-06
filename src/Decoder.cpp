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

#include "DmScanLibInternal.h"
#include "Decoder.h"
#include "Dib.h"

#include <glog/logging.h>
#include <stdio.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

using namespace std;

Decoder::Decoder(unsigned _dpi, double g, unsigned s, unsigned t, unsigned c,
                 double dist)
                : dpi(_dpi), scanGap(g), squareDev(s), edgeThresh(t), corrections(
                                c), cellDistance(dist) {
}

Decoder::~Decoder() {
}

void Decoder::decodeImage(std::tr1::shared_ptr<const Dib> dib,
                          const std::string & id, DecodeResult & decodeResult) {
    int minEdgeSize, maxEdgeSize;

    DmtxImage * dmtxImage = dib->getDmtxImage();
    DmtxDecode *dec = dmtxDecodeCreate(dmtxImage, 1);
    CHECK_NOTNULL(dec);


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
            getDecodeInfo(dec, reg, msg, decodeResult);

            if (GCC_EXT VLOG_IS_ON(2)) {
                showStats(dec, reg, msg);
            }
            dmtxMessageDestroy(&msg);
        }
        dmtxRegionDestroy(&reg);
    }

    if (GCC_EXT VLOG_IS_ON(5)) {
        writeDiagnosticImage(dec, id);
    }

    dmtxDecodeDestroy(&dec);
    dmtxImageDestroy(&dmtxImage);
}

void Decoder::getDecodeInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg,
                            DecodeResult & decodeResult) {
    CHECK_NOTNULL(dec);
    CHECK_NOTNULL(reg);
    CHECK_NOTNULL(msg);

    DmtxVector2 p00, p10, p11, p01;

    decodeResult.msg.assign((char *) msg->output, msg->outputIdx);

    int height = dmtxDecodeGetProp(dec, DmtxPropHeight);
    p00.X = p00.Y = p10.Y = p01.X = 0.0;
    p10.X = p01.Y = p11.X = p11.Y = 1.0;
    dmtxMatrix3VMultiplyBy(&p00, reg->fit2raw);
    dmtxMatrix3VMultiplyBy(&p10, reg->fit2raw);
    dmtxMatrix3VMultiplyBy(&p11, reg->fit2raw);
    dmtxMatrix3VMultiplyBy(&p01, reg->fit2raw);

    p00.Y = height - 1 - p00.Y;
    p10.Y = height - 1 - p10.Y;
    p11.Y = height - 1 - p11.Y;
    p01.Y = height - 1 - p01.Y;

    DmtxVector2 * p[4] = { &p00, &p10, &p11, &p01 };

    for (unsigned i = 0; i < 4; ++i) {
        decodeResult.corners[i].x = static_cast<int>(p[i]->X);
        decodeResult.corners[i].y = static_cast<int>(p[i]->Y);
    }
}

void Decoder::showStats(DmtxDecode * dec, DmtxRegion * reg, DmtxMessage * msg) {
    if (GCC_EXT !VLOG_IS_ON(5)) return;

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

    GCC_EXT VLOG(5)
                    << "\n--------------------------------------------------"
                    << "\n       Matrix Size: "
                    << dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows,
                                              reg->sizeIdx)
                    << " x "
                    << dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols,
                                              reg->sizeIdx)
                    << "\n    Data Codewords: "
                    << dataWordLength - msg->padCount
                    << " (capacity "
                    << dataWordLength
                    << ")"
                    << "\n   Error Codewords: "
                    << dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords,
                                              reg->sizeIdx)
                    << "\n      Data Regions: "
                    << dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions,
                                              reg->sizeIdx)
                    << " x "
                    << dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions,
                                              reg->sizeIdx)
                    << "\nInterleaved Blocks: "
                    << dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks,
                                              reg->sizeIdx)
                    << "\n    Rotation Angle: " << rotateInt
                    << "\n          Corner 0: (" << p00.X << ", "
                    << height - 1 - p00.Y << ")" << "\n          Corner 1: ("
                    << p10.X << ", " << height - 1 - p10.Y << ")"
                    << "\n          Corner 2: (" << p11.X << ", "
                    << height - 1 - p11.Y << ")" << "\n          Corner 3: ("
                    << p01.X << ", " << height - 1 - p01.Y << ")"
                    << "\n--------------------------------------------------";
}

void Decoder::writeDiagnosticImage(DmtxDecode *dec, const std::string & id) {
    if (GCC_EXT !VLOG_IS_ON(5)) return;

    int totalBytes, headerBytes;
    int bytesWritten;
    unsigned char *pnm;
    FILE *fp;

    ostringstream fname;
    fname << "diagnostic-" << id << ".pnm";

    fp = fopen(fname.str().c_str(), "wb");
    CHECK_NOTNULL(fp);

    pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
    CHECK_NOTNULL(pnm);

    bytesWritten = fwrite(pnm, sizeof(unsigned char), totalBytes, fp);
    CHECK(bytesWritten == totalBytes);

    free(pnm);
    fclose(fp);
}
