/*
 * Decoder.cpp
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "Decoder.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Dib.h"
#include "Util.h"
#include "BarcodeInfo.h"
#include "BinRegion.h"

#include <iostream>
#include <string.h>
#include <math.h>
#include <string>
#include <sstream>
#include <limits>

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

using namespace std;

Decoder::Decoder() {
	ua::Logger::Instance().subSysHeaderSet(3, "Decoder");
}

Decoder::~Decoder() {
}

void Decoder::processImage(Dib & dib, vector<BarcodeInfo *>  & msgInfos){
	DmtxImage * image = createDmtxImageFromDib(dib);
	processImage(*image, msgInfos);
	dmtxImageDestroy(&image);
}

void Decoder::processImage(DmtxImage & image, vector<BarcodeInfo *>  & msgInfos) {
	DmtxDecode * dec = NULL;
	DmtxRegion * reg = NULL;
	DmtxMessage * msg = NULL;
	unsigned width = dmtxImageGetProp(&image, DmtxPropWidth);
	unsigned height = dmtxImageGetProp(&image, DmtxPropHeight);

	UA_DOUT(3, 3, "processImage: image width/" << width
			<< " image height/" << height
			<< " row padding/" << dmtxImageGetProp(&image, DmtxPropRowPadBytes)
			<< " image bits per pixel/"
			<< dmtxImageGetProp(&image, DmtxPropBitsPerPixel)
			<< " image row size bytes/"
			<< dmtxImageGetProp(&image, DmtxPropRowSizeBytes));

	dec = dmtxDecodeCreate(&image, 1);
	assert(dec != NULL);

	// save image to a PNM file
	UA_DEBUG(
			FILE * fh;
			unsigned char *pnm;
			int totalBytes;
			int headerBytes;

			pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
			fh = fopen("out.pnm", "w");
			fwrite(pnm, sizeof(unsigned char), totalBytes, fh);
			fclose(fh);
			free(pnm);
	);

	dmtxDecodeSetProp(dec, DmtxPropScanGap, 0);
	dmtxDecodeSetProp(dec, DmtxPropSquareDevn, 10);
	dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, 37);

	unsigned regionCount = 0;
	while (1) {
		reg = dmtxRegionFindNext(dec, NULL);
		if (reg == NULL) break;

		UA_DOUT(3, 5, "retrieving message from region " << regionCount++);
		msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
		if (msg != NULL) {
			BarcodeInfo * info = new BarcodeInfo(dec, reg, msg);
			UA_ASSERT_NOT_NULL(info);

			//showStats(dec, reg, msg);
			msgInfos.push_back(info);
			UA_DOUT(3, 5, "message " << msgInfos.size() - 1
					<< ": "	<< msgInfos.back()->getMsg());
			//showStats(dec, reg, msg);
			dmtxMessageDestroy(&msg);
		}
		dmtxRegionDestroy(&reg);
	}

	dmtxDecodeDestroy(&dec);
}

void Decoder::showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) {
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

	dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords, reg->sizeIdx);

	rotate = (2 * M_PI) + (atan2(reg->fit2raw[0][1], reg->fit2raw[1][1]) -
			atan2(reg->fit2raw[1][0], reg->fit2raw[0][0])) / 2.0;

	rotateInt = (int)(rotate * 180/M_PI + 0.5);
	if(rotateInt >= 360)
		rotateInt -= 360;

	fprintf(stdout, "--------------------------------------------------\n");
	fprintf(stdout, "       Matrix Size: %d x %d\n",
			dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx),
			dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx));
	fprintf(stdout, "    Data Codewords: %d (capacity %d)\n",
			dataWordLength - msg->padCount, dataWordLength);
	fprintf(stdout, "   Error Codewords: %d\n",
			dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, reg->sizeIdx));
	fprintf(stdout, "      Data Regions: %d x %d\n",
			dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, reg->sizeIdx),
			dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, reg->sizeIdx));
	fprintf(stdout, "Interleaved Blocks: %d\n",
			dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, reg->sizeIdx));
	fprintf(stdout, "    Rotation Angle: %d\n", rotateInt);
	fprintf(stdout, "          Corner 0: (%0.1f, %0.1f)\n", p00.X, height - 1 - p00.Y);
	fprintf(stdout, "          Corner 1: (%0.1f, %0.1f)\n", p10.X, height - 1 - p10.Y);
	fprintf(stdout, "          Corner 2: (%0.1f, %0.1f)\n", p11.X, height - 1 - p11.Y);
	fprintf(stdout, "          Corner 3: (%0.1f, %0.1f)\n", p01.X, height - 1 - p01.Y);
	fprintf(stdout, "--------------------------------------------------\n");
}

/*
 *	createDmtxImageFromDib
 *
 */
DmtxImage * Decoder::createDmtxImageFromDib(Dib & dib) {
	int pack = DmtxPack24bppRGB;

	if (dib.getBitsPerPixel() == 32) {
		pack = DmtxPack32bppXRGB;
	}

	// create dmtxImage from the dib
	DmtxImage * image = dmtxImageCreate(dib.getPixelBuffer(), dib.getWidth(),
			dib.getHeight(), pack);

	//set the properties (pad bytes, flip)
	dmtxImageSetProp(image, DmtxPropRowPadBytes, dib.getRowPadBytes());
	dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
	return image;
}/*
 * Should only be called after regions are loaded from INI file.
 */
void Decoder::processImageRegions(unsigned plateNum, Dib & dib,
		vector<DecodeRegion *> & decodeRegions) {
	if (decodeRegions.size() == 0) {
		UA_WARN("no decoded regions; exiting.");
		return;
	}

	vector<BarcodeInfo *> msgInfos;

	for (unsigned i = 0, n = decodeRegions.size(); i < n; ++i) {
		DecodeRegion & region = *decodeRegions[i];
		Dib croppedDib;
		croppedDib.crop(dib, region.topLeft.X, region.topLeft.Y,
				region.botRight.X, region.botRight.Y);
		msgInfos.clear();
		UA_DOUT(3, 3, "processing region at row/" << region.row << " col/" << region.col);
		processImage(croppedDib, msgInfos);
		unsigned size = msgInfos.size();
		UA_ASSERT(size <= 1);
		if (size == 1) {
			region.msgInfo = msgInfos[0];
			UA_DOUT(3, 3, "barcode found at row/" << region.row
					<< " col/" << region.col << " barcode/" << region.msgInfo->getMsg());
		}
	}
}

void Decoder::imageShowRegions(Dib & dib, vector<DecodeRegion *> & decodeRegions) {
	UA_DOUT(4, 3, "marking tags ");

	RgbQuad quadRed(255, 0, 0);
	RgbQuad quadGreen(0, 255, 0);

	for (unsigned i = 0, n = decodeRegions.size(); i < n; ++i) {
		DecodeRegion & region = *decodeRegions[i];

		RgbQuad & quad = (region.msgInfo == NULL) ? quadRed : quadGreen;

		dib.line(region.topLeft.X, region.topLeft.Y,
				region.topLeft.X, region.botRight.Y, quad);

		dib.line(region.topLeft.X, region.botRight.Y,
				region.botRight.X, region.botRight.Y, quad);

		dib.line(region.botRight.X, region.botRight.Y,
				region.botRight.X, region.topLeft.Y, quad);

		dib.line(region.botRight.X, region.topLeft.Y,
				region.topLeft.X, region.topLeft.Y, quad);

	}
}

ostream & operator<<(ostream &os, DecodeRegion & r) {
	os << r.row	<< "," << r.col << ": "
		<< "(" << r.topLeft.X << ", " << r.topLeft.Y << "), "
		<< "(" << r.botRight.X << ", " << r.botRight.Y << ")";
	return os;
}
