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

#include "BarcodeThread.h"
#include "BarcodeInfo.h"
#include "Dib.h"
#include "dmtx.h"
#include "Decoder.h"
#include "UaAssert.h"
#include "UaLogger.h"
#include "BarcodeInfo.h"
#include "ProcessImageManager.h"
#include "cxtypes.h"

#include <stdio.h>
#include <sstream>

BarcodeThread::BarcodeThread(ProcessImageManager * manager, double scanGap,
        unsigned squareDev, unsigned edgeThresh, unsigned corrections,
        CvRect & croppedOffset, auto_ptr<Dib> d, BarcodeInfo & info) :
        dib(d), image(Decoder::createDmtxImageFromDib(*dib)), barcodeInfo(info),
            debug(true) {


    this->manager = manager;
    this->scanGap = scanGap;
    this->squareDev = squareDev;
    this->edgeThresh = edgeThresh;
    this->corrections = corrections;
    this->croppedOffset = croppedOffset;

    quitMutex.lock();
    this->quitFlag = false;
    quitMutex.unlock();


    dpi = dib->getDpi();

    UA_ASSERTS((dpi == 300) || (dpi == 400) || (dpi == 600),
            "invalid DPI: " << dpi);

    // do not write diagnostic image is log level is less than 9
    if (ua::Logger::Instance().levelGet(3) >= 9) {
        ostringstream fname;
        CvRect & rect = barcodeInfo.getPreProcessBoundingBox();
        fname << "preprocess-" << rect.x << "-" << rect.y << ".bmp";
        dib->writeToFile(fname.str().c_str());
    }
}

BarcodeThread::~BarcodeThread() {
    dmtxImageDestroy(&image);
}

void BarcodeThread::run() {
    int minEdgeSize, maxEdgeSize;

    DmtxDecode *dec = dmtxDecodeCreate(image, 1);
    UA_ASSERT_NOT_NULL(dec);

    // slightly smaller than the new tube edge
    minEdgeSize = (int)(0.08 * dpi);

    // slightly bigger than the Nunc edge
    maxEdgeSize =(int)(0.18 * dpi);

    dmtxDecodeSetProp(dec, DmtxPropEdgeMin, minEdgeSize);
    dmtxDecodeSetProp(dec, DmtxPropEdgeMax, maxEdgeSize);
    dmtxDecodeSetProp(dec, DmtxPropSymbolSize, DmtxSymbolSquareAuto);
    dmtxDecodeSetProp(dec, DmtxPropScanGap, static_cast<unsigned> (scanGap
            * dpi));
    dmtxDecodeSetProp(dec, DmtxPropSquareDevn, squareDev);
    dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, edgeThresh);

    UA_DOUT(3, 7, "BarcodeThread: image (" << croppedOffset.x
    		<< "," << croppedOffset.y
    		<< ") width/" << image->width
            << " image height/" << image->height << " row padding/"
            << dmtxImageGetProp(image, DmtxPropRowPadBytes)
            << " image bits per pixel/"
            << dmtxImageGetProp(image, DmtxPropBitsPerPixel)
            << " image row size bytes/"
            << dmtxImageGetProp(image, DmtxPropRowSizeBytes));

    if ((croppedOffset.x == 442) && (croppedOffset.y == 47)) {
        UA_DOUT(3, 7, "here");
    }

    bool msgFound = false;
    while (1) {
        DmtxRegion *reg = NULL;

        reg = dmtxRegionFindNext(dec, NULL);
        if (reg == NULL) break;

        DmtxMessage *msg = dmtxDecodeMatrixRegion(dec, reg, corrections);
        if (msg != NULL) {
        	msgFound = true;
            barcodeInfo.postProcess(dec, reg, msg);

            if ((croppedOffset.width != 0) && (croppedOffset.height != 0)) {
            	// translate the barcode region information to the actual
            	// image coordinates
                barcodeInfo.translate(croppedOffset.x, croppedOffset.y);
            }

            CvRect & rect = barcodeInfo.getPostProcessBoundingBox();

            UA_DOUT(3, 5, "BarcodeThread: message/"
                    << barcodeInfo.getMsg()
                    << " tlCorner/(" << rect.x << "," << rect.y
                    << ")  brCorner/(" << rect.x + rect.width
                    << "," << rect.y + rect.height << ")");
            dmtxMessageDestroy(&msg);
        }
        dmtxRegionDestroy(&reg);
    }

	if (ua::Logger::Instance().levelGet(3) >= 9) {
    	if (msgFound) {
    		writeDib("found");
    	} else {
    		writeDib("missed");
    	}
        writeDiagnosticImage(dec);
    }

    dmtxDecodeDestroy(&dec);

    quitMutex.lock();
    quitFlag = true;
    quitMutex.unlock();
    return;
}

bool BarcodeThread::isFinished() {
    bool quitFlagBuf;

    quitMutex.lock();
    quitFlagBuf = quitFlag;
    quitMutex.unlock();

    return quitFlagBuf;
}

void BarcodeThread::writeDib(const char * basename) {
	// do not write diagnostic image is log level is less than 9
	if (ua::Logger::Instance().levelGet(3) < 9) return;

    ostringstream fname;
    fname << basename << "-" << croppedOffset.x << "-" << croppedOffset.y << ".bmp";
    dib->writeToFile(fname.str().c_str());
}

void BarcodeThread::writeDiagnosticImage(DmtxDecode *dec) {
	// do not write diagnostic image is log level is less than 9
	if (ua::Logger::Instance().levelGet(3) < 9) return;

    int totalBytes, headerBytes;
    int bytesWritten;
    unsigned char *pnm;
    FILE *fp;

    ostringstream fname;
    CvRect & rect = barcodeInfo.getPreProcessBoundingBox();
    fname << "diagnostic-" << rect.x << "-" << rect.y << ".pnm";

    fp = fopen(fname.str().c_str(), "wb");
    UA_ASSERT_NOT_NULL(fp);

    pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
    UA_ASSERT_NOT_NULL(pnm);

    bytesWritten = fwrite(pnm, sizeof(unsigned char), totalBytes, fp);
    UA_ASSERT(bytesWritten == totalBytes);

    free(pnm);
    fclose(fp);
}

