/*
 * Decoder.cpp
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "Decoder.h"
#include "UaDebug.h"
#include "Dib.h"

#include <iostream>
#include <string.h>
#include <math.h>
#include <string>
#include <sstream>

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

using namespace std;

struct MessageInfo {
	char * str;
	DmtxVector2 p00, p10, p11, p01;
};

struct RegionRect {
	DmtxPixelLoc corner0;
	DmtxPixelLoc corner1;
	unsigned rank;

	RegionRect() {
		corner0.X = corner0.Y = corner1.X = corner1.Y = 0;
		rank = 0;
	}

	RegionRect (int x0, int y0, int x1, int y1) {
		corner0.X = x0; corner0.Y = y0;
		corner1.X = x1; corner1.Y = y1;
		rank = 0;
	}

	string toString() {
		ostringstream out;
		out << " (" << corner0.X << "," << corner0.Y << ")"
		<< " (" << corner1.X << "," << corner1.Y << ")";
		return out.str();
	}
};

struct RegionRectSort {
	// use distance from origin
	bool operator()(RegionRect* const& a, RegionRect* const& b) {
		unsigned aDistSquared = a->corner0.X *  a->corner0.X + a->corner0.Y * a->corner0.Y;
		unsigned bDistSquared = b->corner0.X *  b->corner0.X + b->corner0.Y * b->corner0.Y;

		return (aDistSquared < bDistSquared);
	}
};

Decoder::Decoder() {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(1, "Decoder"));
}

Decoder::Decoder(Dib & dib) {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(1, "Decoder"));
	decodeImage(dib);
}

Decoder::Decoder(DmtxImage & image) {
	UA_DEBUG(ua::Debug::Instance().subSysHeaderSet(1, "Decoder"));
	decodeImage(image);
}

Decoder::~Decoder() {
	clearResults();
}

void Decoder::clearResults() {
	RegionRect * rect;

	while (results.size() > 0) {
		MessageInfo * info = results.back();
		results.pop_back();
		delete [] info->str;
		delete info;
	}
	while (rowRegions.size() > 0) {
		rect = rowRegions.back();
		rowRegions.pop_back();
		delete rect;
	}
	while (colRegions.size() > 0) {
		rect = colRegions.back();
		colRegions.pop_back();
		delete rect;
	}
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
void Decoder::decodeImage(Dib & dib){
	DmtxImage * image = createDmtxImageFromDib(dib);
	decodeImage(*image);
	dmtxImageDestroy(&image);
}

void Decoder::decodeImage(DmtxImage & image) {
	if (results.size() > 0) {
		// an image was already created, destroy this one as a new one
		// is created below
		clearResults();
	}

	DmtxDecode * dec = NULL;
	DmtxRegion * reg = NULL;
	DmtxMessage * msg = NULL;
	FILE * fh;
	unsigned char *pnm;
	int totalBytes, headerBytes;
	unsigned width = dmtxImageGetProp(&image, DmtxPropWidth);
	unsigned height = dmtxImageGetProp(&image, DmtxPropHeight);

	UA_DOUT(1, 3, "decodeImage: image width/" << width
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

		UA_DOUT(1, 3, "retrieving message from region " << regionCount++);
		msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
		if (msg != NULL) {
			messageAdd(dec, reg, msg);
			UA_DOUT(1, 3, "message " << results.size() - 1
					<< ": "	<< results.back()->str);
			//showStats(dec, reg, msg);
			dmtxMessageDestroy(&msg);
		}
		dmtxRegionDestroy(&reg);
	}

	dmtxDecodeDestroy(&dec);
	sortRegions(height, width);
}

void Decoder::messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) {
	MessageInfo * info = new MessageInfo;
	UA_ASSERT_NOT_NULL(info);
	info->str = new char[msg->outputIdx + 1];
	UA_ASSERT_NOT_NULL(info->str);
	memcpy(info->str, msg->output, msg->outputIdx);
	info->str[msg->outputIdx] = 0;

	int height = dmtxDecodeGetProp(dec, DmtxPropHeight);
	info->p00.X = info->p00.Y = info->p10.Y = info->p01.X = 0.0;
	info->p10.X = info->p01.Y = info->p11.X = info->p11.Y = 1.0;
	dmtxMatrix3VMultiplyBy(&info->p00, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&info->p10, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&info->p11, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&info->p01, reg->fit2raw);

	info->p00.Y = height - 1 - info->p00.Y;
	info->p10.Y = height - 1 - info->p10.Y;
	info->p11.Y = height - 1 - info->p11.Y;
	info->p01.Y = height - 1 - info->p01.Y;

	results.push_back(info);
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
}

unsigned Decoder::getNumTags() {
	return results.size();
}

char * Decoder::getTag(unsigned tagNum) {
	UA_ASSERT(tagNum < results.size());
	return results[tagNum]->str;
}

void Decoder::getTagCorners(int tagNum, DmtxVector2 & p00, DmtxVector2 & p10,
		DmtxVector2 & p11, DmtxVector2 & p01) {
	MessageInfo & info = *results[tagNum];
	p00 = info.p00;
	p10 = info.p10;
	p11 = info.p11;
	p01 = info.p01;
}

void Decoder::debugShowTags() {
	unsigned numTags = results.size();
	UA_DOUT(1, 1, "debugTags: tags found: " << numTags);
	for (unsigned i = 0; i < numTags; ++i) {
		MessageInfo & info = *results[i];
		UA_DOUT(1, 1, "debugTags: tag " << i << ": " << info.str
				<< ", corners: (" << info.p00.X << ", " << info.p00.Y << "), "
				<< "(" << info.p10.X << ", " << info.p10.Y << "), "
				<< "(" << info.p11.X << ", " << info.p11.Y << "), "
				<< "(" << info.p01.X << ", " << info.p01.Y << ")");
	}
}

void Decoder::sortRegions(unsigned imageHeight, unsigned imageWidth) {
	bool rowRegionFound;
	bool colRegionFound;

	for (unsigned i = 0, n = results.size(); i < n; ++i) {
		MessageInfo & info = *results[i];
		rowRegionFound = false;
		colRegionFound = false;
		int top = (int) (info.p00.Y > info.p00.Y ? info.p00.Y : info.p00.Y);
		int bot = (int) (info.p10.Y > info.p11.Y ? info.p10.Y : info.p11.Y);
		int left  = (int) (info.p00.X > info.p10.X ? info.p00.X : info.p00.X);
		int right = (int) (info.p11.X > info.p01.X ? info.p11.X : info.p01.X);

		UA_DOUT(1, 9, "tag " << i << " : top/" << top << " bottom/" << bot
				<< " left/" << left << " right/" << right);

		for (unsigned c = 0, cn = colRegions.size(); c < cn; ++c) {
			RegionRect & colRegion = *colRegions[c];
			int leftDiff = colRegion.corner0.X - left;
			int rightDiff = colRegion.corner1.X - right;
			UA_DOUT(1, 9, "col " << c << ": leftDiff/" << leftDiff << " rightDiff/" << rightDiff);

			if ((leftDiff <= 0) && (rightDiff >= 0)) {
				colRegionFound = true;
			}

			if ((leftDiff > 0) && (leftDiff < REGION_PIX_THRESH)) {
				colRegion.corner0.X = left;
				UA_DOUT(1, 9, "col " << c << ": new left: " << left);
				colRegionFound = true;
			}

			if ((rightDiff < 0) && (rightDiff > -REGION_PIX_THRESH)) {
				colRegion.corner1.X = right;
				UA_DOUT(1, 9, "col " << c << ": new right: " << right);
				colRegionFound = true;
			}
		}

		for (unsigned r = 0, rn = rowRegions.size(); r < rn; ++r) {
			RegionRect & rowRegion = *rowRegions[r];
			int topDiff = rowRegion.corner0.Y - top;
			int botDiff = rowRegion.corner1.Y - bot;
			UA_DOUT(1, 9, "row " << r << ": topDiff/" << topDiff << " botDiff/" << botDiff);

			if ((topDiff <= 0) && (botDiff >= 0)) {
				rowRegionFound = true;
			}

			if ((topDiff > 0) && (topDiff < REGION_PIX_THRESH)) {
				rowRegion.corner0.Y = top;
				UA_DOUT(1, 9, "row " << r << ": new top: " << top);
				rowRegionFound = true;
			}

			if ((botDiff < 0) && (botDiff > -REGION_PIX_THRESH)) {
				rowRegion.corner1.Y = bot;
				UA_DOUT(1, 9, "row " << r << ": new bot: " << bot);
				rowRegionFound = true;
			}
		}

		if (!colRegionFound) {
			RegionRect * newRegion = new RegionRect(left, 0, right, imageHeight
					);
			UA_DOUT(1, 9, "new col " << colRegions.size() << ": left/" << left << " right/" << right);
			colRegions.push_back(newRegion);
		}

		if (!rowRegionFound) {
			RegionRect * newRegion = new RegionRect(0, top, imageWidth, bot);
			UA_DOUT(1, 9, "new row " << rowRegions.size() << ": top/" << top << " bot/" << bot);
			rowRegions.push_back(newRegion);
		}
	}

	sort(rowRegions.begin(), rowRegions.end(), RegionRectSort());
	sort(colRegions.begin(), colRegions.end(), RegionRectSort());

	for (unsigned i = 0, n = rowRegions.size(); i < n; ++ i) {
		RegionRect & r = *rowRegions[i];
		r.rank = i;
		UA_DOUT(1, 1, "row region " << i
				<< " (" << r.corner0.X << "," << r.corner0.Y << ")"
				<< " (" << r.corner1.X << "," << r.corner1.Y << ")");
	}

	for (unsigned i = 0, n = colRegions.size(); i < n; ++ i) {
		RegionRect & r = *colRegions[i];
		r.rank = i;
		UA_DOUT(1, 1, "col region " << i << r.toString());
	}
}
