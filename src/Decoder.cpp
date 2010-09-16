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

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

#include "Decoder.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Dib.h"
#include "BarcodeInfo.h"
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

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

using namespace std;

const double Decoder::BARCODE_SIDE_LENGTH_INCHES = 0.13;

Decoder::Decoder(double g, unsigned s, unsigned t, unsigned c, double dist,
        PalletGrid * pg) :
    palletGrid(pg) {
    ua::Logger::Instance().subSysHeaderSet(3, "Decoder");
    scanGap = g;
    squareDev = s;
    edgeThresh = t;
    corrections = c;
    cellDistance = dist;

    barcodeInfos.resize(PalletGrid::MAX_ROWS);
    for (unsigned row = 0; row < PalletGrid::MAX_ROWS; ++row) {
        barcodeInfos[row].resize(PalletGrid::MAX_COLS);
    }
}

Decoder::~Decoder() {
    for (unsigned row = 0; row < PalletGrid::MAX_ROWS; row++) {
        for (unsigned col = 0; col < PalletGrid::MAX_COLS; col++) {
            if (barcodeInfos[row][col] != NULL) {
                delete barcodeInfos[row][col];
            }
        }
    }
}

// reduces the blob to a smaller region (matrix outline)
bool Decoder::reduceBlobToMatrix(Dib * dib, CvRect & inputBlob) {
    Dib croppedDib;
    IplImageContainer *img;

    croppedDib.crop(*dib, inputBlob.x, inputBlob.y, inputBlob.x
            + inputBlob.width, inputBlob.y + inputBlob.height);
    UA_ASSERT_NOT_NULL(croppedDib.getPixelBuffer());

    img = croppedDib.generateIplImage();
    UA_ASSERT_NOT_NULL(img);
    UA_ASSERT_NOT_NULL(img->getIplImage());

    for (int i = 0; i < 5; i++)
        cvSmooth(img->getIplImage(), img->getIplImage(), CV_GAUSSIAN, 11, 11);
    cvThreshold(img->getIplImage(), img->getIplImage(), 50, 255,
            CV_THRESH_BINARY);

    CBlobResult blobs(img->getIplImage(), NULL, 0);

    switch (img->getHorizontalResolution()) {
    case 300:
        blobs.Filter(blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 840);
        break;
    case 400:
        blobs.Filter(blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 1900);
        break;
    case 600:
        blobs.Filter(blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, 2400);
        break;
    }
    delete img;

    /* ---- Grabs the largest blob in the blobs vector -----*/
    bool reducedBlob = false;
    CvRect largestBlob =
        { 0, 0, 0, 0 };

    for (int i = 0; i < blobs.GetNumBlobs(); i++) {
        CvRect & currentBlob = blobs.GetBlob(i)->GetBoundingBox();
        if (currentBlob.width * currentBlob.height > largestBlob.width
                * largestBlob.height) {
            largestBlob = currentBlob;
            reducedBlob = true;
        }
    }

    /* ---- Keep blob that was successfully reduced-----*/
    if (reducedBlob) {
        largestBlob.x += inputBlob.x;
        largestBlob.y += inputBlob.y;
        inputBlob = largestBlob;
    }
    return reducedBlob;

}

Decoder::ProcessResult Decoder::processImageRegions(Dib * dib) {
    dpi = dib->getDpi();
    double minBlobWidth = dpi * BARCODE_SIDE_LENGTH_INCHES;
    double minBlobHeight = dpi * BARCODE_SIDE_LENGTH_INCHES;
    CvRect rect;

    UA_ASSERT_NOT_NULL(palletGrid);
    UA_ASSERT(palletGrid->isImageValid());

#ifdef _DEBUG
    ostringstream out;
    for (unsigned row = 0; row < PalletGrid::MAX_ROWS; ++row) {
        for (unsigned col = 0; col < PalletGrid::MAX_COLS; ++col) {
            out << palletGrid->getCellEnabled(row, col);
        }
        out << endl;
    }
    UA_DOUT(1, 5, "Loaded Profile: \n" << out.str());
#endif

    UA_DOUT(1, 7, "Minimum blob width (pixels): " << minBlobWidth);
    UA_DOUT(1, 7, "Minimum blob height (pixels): " << minBlobHeight);

    // generate blobs
    unsigned blobRegionCount = 0;
    for (unsigned row = 0; row < PalletGrid::MAX_ROWS; row++) {
        for (unsigned col = 0; col < PalletGrid::MAX_COLS; col++) {
            if (!palletGrid->getCellEnabled(row, col)) continue;

            palletGrid->getImageCoordinates(row, col, rect);
            UA_DOUT(1, 9, "row/" << row << " col/" << col << " rect/("
                    << rect.x << ", " << rect.y << "),(" << rect.x + rect.width
                    << ", " << rect.y + rect.height << ")");
            if (!reduceBlobToMatrix(dib, rect)) continue;

            BarcodeInfo * info = new BarcodeInfo();
            info->setPreProcessBoundingBox(rect);
            barcodeInfos[row][col] = info;
            ++blobRegionCount;
        }
    }
    UA_DOUT(1, 7, "blob regions found: " << blobRegionCount);

    if (blobRegionCount == 0) {
    	// no blobs found
        return IMG_INVALID;
    }

#ifdef _DEBUG
    // record blob regions
    Dib blobDib(*dib);
    RgbQuad white(255, 255, 255);
    for (unsigned row = 0, rows = barcodeInfos.size(); row < rows; ++row) {
        for (unsigned col = 0, cols = barcodeInfos[row].size(); col < cols; ++col) {
            BarcodeInfo * info = barcodeInfos[row][col];
        	if (info == NULL) continue;

        	CvRect & rect = barcodeInfos[row][col]->getPreProcessBoundingBox();
        	blobDib.rectangle(rect.x, rect.y, rect.width, rect.height, white);
        }
    }
    blobDib.writeToFile("blobRegions.bmp");
#endif

    ProcessImageManager imageProcessor(this, scanGap, squareDev, edgeThresh,
            corrections);
    imageProcessor.generateBarcodes(dib, barcodeInfos);

    unsigned decodeCount = 0;
    for (unsigned row = 0, rows = barcodeInfos.size(); row < rows; ++row) {
        for (unsigned col = 0, cols = barcodeInfos[row].size(); col < cols; ++col) {
            BarcodeInfo * info = barcodeInfos[row][col];
        	if ((info != NULL) && info->isValid()) {
                ++decodeCount;
            }
        }
    }

    if (decodeCount == 0) {
        UA_DOUT(3, 5, "no barcodes were found.");
        return IMG_INVALID;
    }

    return OK;
}

DmtxImage * Decoder::createDmtxImageFromDib(Dib & dib) {
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

    DmtxImage *image = dmtxImageCreate(dib.getPixelBuffer(), dib.getWidth(),
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

    rotate = (2 * M_PI) + (atan2(reg->fit2raw[0][1], reg->fit2raw[1][1])
            - atan2(reg->fit2raw[1][0], reg->fit2raw[0][0])) / 2.0;

    rotateInt = (int) (rotate * 180 / M_PI + 0.5);
    if (rotateInt >= 360) rotateInt -= 360;

    fprintf(stdout, "--------------------------------------------------\n");
    fprintf(stdout, "       Matrix Size: %d x %d\n", dmtxGetSymbolAttribute(
            DmtxSymAttribSymbolRows, reg->sizeIdx), dmtxGetSymbolAttribute(
            DmtxSymAttribSymbolCols, reg->sizeIdx));
    fprintf(stdout, "    Data Codewords: %d (capacity %d)\n", dataWordLength
            - msg->padCount, dataWordLength);
    fprintf(stdout, "   Error Codewords: %d\n", dmtxGetSymbolAttribute(
            DmtxSymAttribSymbolErrorWords, reg->sizeIdx));
    fprintf(stdout, "      Data Regions: %d x %d\n", dmtxGetSymbolAttribute(
            DmtxSymAttribHorizDataRegions, reg->sizeIdx),
            dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, reg->sizeIdx));
    fprintf(stdout, "Interleaved Blocks: %d\n", dmtxGetSymbolAttribute(
            DmtxSymAttribInterleavedBlocks, reg->sizeIdx));
    fprintf(stdout, "    Rotation Angle: %d\n", rotateInt);
    fprintf(stdout, "          Corner 0: (%0.1f, %0.1f)\n", p00.X, height - 1
            - p00.Y);
    fprintf(stdout, "          Corner 1: (%0.1f, %0.1f)\n", p10.X, height - 1
            - p10.Y);
    fprintf(stdout, "          Corner 2: (%0.1f, %0.1f)\n", p11.X, height - 1
            - p11.Y);
    fprintf(stdout, "          Corner 3: (%0.1f, %0.1f)\n", p01.X, height - 1
            - p01.Y);
    fprintf(stdout, "--------------------------------------------------\n");
}

void Decoder::imageShowBarcodes(Dib & dib, bool regions) {
    UA_DOUT(3, 3, "marking tags ");
    if (barcodeInfos.empty()) return;

    RgbQuad quadWhite(255, 255, 255); // change to white (shows up better in grayscale)
    RgbQuad quadPink(255, 0, 255);
    RgbQuad quadRed(255, 0, 0);

    RgbQuad & highlightQuad = (dib.getBitsPerPixel() == 8 ? quadWhite
            : quadPink);

    map<CvPoint, BarcodeInfo>::iterator it;
    for (unsigned row = 0, rows = barcodeInfos.size(); row < rows; ++row) {
        for (unsigned col = 0, cols = barcodeInfos[row].size(); col < cols; ++col) {
            BarcodeInfo * info = barcodeInfos[row][col];
            if ((info == NULL) || !info->isValid()) continue;

            CvRect rect = barcodeInfos[row][col]->getPostProcessBoundingBox();
            dib.rectangle(rect.x, rect.y, rect.width, rect.height,
                    highlightQuad);
        }
    }
}

const char * Decoder::getBarcode(unsigned row, unsigned col) {
    UA_ASSERT(row < PalletGrid::MAX_ROWS);
    UA_ASSERT(col < PalletGrid::MAX_COLS);

    BarcodeInfo * info = barcodeInfos[row][col];
	if ((info == NULL) || !info->isValid()) return NULL;

    return info->getMsg().c_str();
}
