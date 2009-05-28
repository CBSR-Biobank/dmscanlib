/*
 * Decoder.cpp
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "Decoder.h"
#include "UaDebug.h"
#include "Dib.h"
#include "LinkList.h"

#include <iostream>
#include <string.h>
#include <math.h>

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

using namespace std;

Decoder::Decoder(Dib * dib) :
	results(new LinkList()) {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(1, "Decoder"));
	decodeImage(dib);
}

Decoder::Decoder(DmtxImage * image) :
	results(new LinkList()) {
	decodeImage(image);
}

Decoder::~Decoder() {
	delete results;
}

/*
 *	decodeDib
 *	@params - filename: char* corresponding to the filename of an image
 *	@return - The number of bytes that got truncated to fit the barcodes
 *			  in the supplied buffer.
 *
 *	Create a file from the filename given, then create a DmtxImage from this
 *	file. If a DmxtImage can be created, decode it. All barcodes decoded are
 *	stored in the supplied buffer, up to a max length of bufferSize.
 */
void Decoder::decodeImage(Dib * dib){
	UA_ASSERT_NOT_NULL(dib);
	UA_ASSERT_NOT_NULL(results);

	DmtxImage * image = createDmtxImageFromDib(dib);
	decodeImage(image);
	dmtxImageDestroy(&image);
}

void Decoder::decodeImage(DmtxImage * image) {
	UA_ASSERT_NOT_NULL(image);

	if (results != NULL) {
		// an image was already created, destroy this one as a new one
		// is created below
		delete results;
	}
	results = new LinkList();

	DmtxDecode * dec = NULL;
	DmtxRegion * reg = NULL;
	DmtxMessage * msg = NULL;
	FILE * fh;
	unsigned char *pnm;
	int totalBytes, headerBytes;

	UA_DOUT(1, 3, "decodeImage: image width/"
			<< dmtxImageGetProp(image, DmtxPropWidth)
			<< " image height/" << dmtxImageGetProp(image, DmtxPropHeight)
			<< " row padding/" << dmtxImageGetProp(image, DmtxPropRowPadBytes)
			<< " image bits per pixel/"
			<< dmtxImageGetProp(image, DmtxPropBitsPerPixel)
			<< " image row size bytes/"
			<< dmtxImageGetProp(image, DmtxPropRowSizeBytes));

	dec = dmtxDecodeCreate(image, 1);
	assert(dec != NULL);

	// save image to a PNM file
	UA_DEBUG(
			pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
			fh = fopen("out.pnm", "w");
			fwrite(pnm, sizeof(unsigned char), totalBytes, fh);
			fclose(fh);
	);

	dmtxDecodeSetProp(dec, DmtxPropScanGap, 0);
	dmtxDecodeSetProp(dec, DmtxPropSquareDevn, 10);
	dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, 37);

	reg = dmtxRegionFindNext(dec, NULL);
	if (reg == NULL) {
		UA_DOUT(1, 3, "first attemt to find region failed");
		// change decode parameters and try to find region again
		dmtxDecodeDestroy(&dec);
		dec = dmtxDecodeCreate(image, 1);
		dmtxDecodeSetProp(dec, DmtxPropScanGap, 0);
		dmtxDecodeSetProp(dec, DmtxPropSquareDevn, 10);
		dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, 10);
		reg = dmtxRegionFindNext(dec, NULL);
		if (reg == NULL) {
			UA_DOUT(1, 3, "no regions found");
			return;
		}
	}

	for (unsigned i = 0; i < NUM_SCANS; ++i) {
		UA_DOUT(1, 3, "retrieving message from region");
		msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
		messageFound(msg->output, msg->outputIdx);
		showStats(dec, reg, msg);
		dmtxMessageDestroy(&msg);

		reg = dmtxRegionFindNext(dec, NULL);
		if (reg == NULL) {
			UA_DOUT(1, 3, "no regions found ...");
			return;
		}
		UA_DOUT(1, 3, "found another region");
	}

	dmtxRegionDestroy(&reg);
	dmtxDecodeDestroy(&dec);
}

void Decoder::messageFound(unsigned char * msg, int msgSize) {
	char * buffer = new char[msgSize + 1];
	UA_ASSERT_NOT_NULL(buffer);
	memcpy(buffer, msg, msgSize);
	buffer[msgSize] = 0;
	results->append(buffer);
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

/**
 *	createDmtxImageFromFile
 *
 *	Open the file and create a DmtxImage out of it.
 *
 *	@params - filename: char* corresponding to the filename of an image
 *			  dib - a blank Divice Independant Bitmap to read the image into
 *	@return - DmtxImage: the newly created image from the file.
 *
 */
DmtxImage * Decoder::createDmtxImageFromDib(Dib * dib) {
	UA_ASSERT_NOT_NULL(dib);

	int pack = DmtxPack24bppRGB;

	if (dib->getBitsPerPixel() == 32) {
		pack = DmtxPack32bppXRGB;
	}

	// create dmtxImage from the dib
	DmtxImage * image = dmtxImageCreate(dib->getPixelBuffer(), dib->getWidth(),
			dib->getHeight(), pack);

	//set the properties (pad bytes, flip)
	dmtxImageSetProp(image, DmtxPropRowPadBytes, dib->getRowPadBytes());
	dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
	return image;
}

unsigned Decoder::getNumTags() {
	UA_ASSERT_NOT_NULL(results);
	return results->size();
}

char * Decoder::getTag(int tagNum) {
	UA_ASSERT_NOT_NULL(results);
	return (char *) results->getItem(tagNum);
}

