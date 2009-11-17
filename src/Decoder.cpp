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
//#include <sstream>
#include <limits>

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

using namespace std;

Decoder::Decoder(unsigned g, unsigned s, unsigned t) {
	ua::Logger::Instance().subSysHeaderSet(3, "Decoder");
	scanGap = g;
	squareDev = s;
	edgeThresh = t;
}

Decoder::~Decoder() {
	BarcodeInfo * b;
	BinRegion * c;

	while (barcodeInfos.size() > 0) {
		b = barcodeInfos.back();
		barcodeInfos.pop_back();
		UA_ASSERT_NOT_NULL(b);
		delete b;
	}

	while (rowBinRegions.size() > 0) {
		c = rowBinRegions.back();
		rowBinRegions.pop_back();
		UA_ASSERT_NOT_NULL(c);
		delete c;
	}
	while (colBinRegions.size() > 0) {
		c = colBinRegions.back();
		colBinRegions.pop_back();
		UA_ASSERT_NOT_NULL(c);
		delete c;
	}
}

/*
 * Should only be called after regions are loaded from INI file.
 */
bool Decoder::processImageRegions(unsigned plateNum, Dib & dib) {
	if (!processImage(dib))
		return false;

	sortRegions();
	return true;
}

bool Decoder::processImage(Dib & dib) {
	DmtxImage & image = *createDmtxImageFromDib(dib);
	DmtxDecode * dec = NULL;
	unsigned width = dmtxImageGetProp(&image, DmtxPropWidth);
	unsigned height = dmtxImageGetProp(&image, DmtxPropHeight);

	UA_DOUT(4, 5, "processImage: image width/" << width
			<< " image height/" << height
			<< " row padding/" << dmtxImageGetProp(&image, DmtxPropRowPadBytes)
			<< " image bits per pixel/"
			<< dmtxImageGetProp(&image, DmtxPropBitsPerPixel)
			<< " image row size bytes/"
			<< dmtxImageGetProp(&image, DmtxPropRowSizeBytes));

	dec = dmtxDecodeCreate(&image, 1);
	UA_ASSERT_NOT_NULL(dec);

	dmtxDecodeSetProp(dec, DmtxPropSymbolSize, DmtxSymbolSquareAuto);
	dmtxDecodeSetProp(dec, DmtxPropScanGap, scanGap);
	dmtxDecodeSetProp(dec, DmtxPropSquareDevn, squareDev);
	dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, edgeThresh);

	unsigned regionCount = 0;
	while (1) {
		if (!decode(dec, 1, barcodeInfos)) {
			break;
		}
		UA_DOUT(4, 5, "retrieved message from region " << regionCount++);
	}

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

	dmtxDecodeDestroy(&dec);

	if (barcodeInfos.size() == 0) {
		UA_DOUT(4, 1, "processImage: no barcodes found");
		return false;
	}
	return true;
}

bool Decoder::decode(DmtxDecode *& dec, unsigned attempts,
		vector<BarcodeInfo *> & barcodeInfos) {
	DmtxRegion * reg = NULL;
	BarcodeInfo * info = NULL;

	for (unsigned i = 0; i < attempts; ++i) {
		reg = dmtxRegionFindNext(dec, NULL);
		if (reg == NULL)
			return false;

		DmtxMessage * msg = dmtxDecodeMatrixRegion(dec, reg, DmtxUndefined);
		if (msg != NULL) {
			info = new BarcodeInfo(dec, reg, msg);
			UA_ASSERT_NOT_NULL(info);
			barcodeInfos.push_back(info);

			DmtxPixelLoc & tlCorner = info->getTopLeftCorner();
			DmtxPixelLoc & brCorner = info->getBotRightCorner();

			UA_DOUT(3, 5, "message " << barcodeInfos.size() - 1
					<< ": " << info->getMsg()
					<< " : tlCorner/" << tlCorner.X << "," << tlCorner.Y
					<< "  brCorner/" << brCorner.X << "," << brCorner.Y);
			//showStats(dec, reg, msg);
			dmtxMessageDestroy(&msg);
			dmtxRegionDestroy(&reg);
			return true;
		}
		dmtxRegionDestroy(&reg);
	}
	return true;
}

void Decoder::sortRegions() {
	unsigned numBarcodes = barcodeInfos.size();
	UA_ASSERTS(numBarcodes != 0, "no barcodes in barcodeInfos vector");

	bool insideRowBin;
	bool insideColBin;

	for (unsigned i = 0, n = barcodeInfos.size(); i < n; ++i) {
		insideRowBin = false;
		insideColBin = false;

		DmtxPixelLoc & tlCorner = barcodeInfos[i]->getTopLeftCorner();
		DmtxPixelLoc & brCorner = barcodeInfos[i]->getBotRightCorner();

		UA_DOUT(4, 9, "tag " << i << " : tlCorner/" << tlCorner.X << "," << tlCorner.Y
				<< "  brCorner/" << brCorner.X << "," << brCorner.Y);

		if (tlCorner.X == 669) {
			UA_DOUT(4, 1, "here");
		}

		for (unsigned c = 0, cn = colBinRegions.size(); c < cn; ++c) {
			BinRegion & bin = *colBinRegions[c];

			unsigned min = bin.getMin();
			unsigned max = bin.getMax();
			unsigned left = static_cast<unsigned>(tlCorner.X);
			unsigned right = static_cast<unsigned>(brCorner.X);

			if (((min - BIN_THRESH <= left) && (left <= max + BIN_THRESH))
					|| ((min - BIN_THRESH <= right) && (right <= max + BIN_THRESH))) {
				insideColBin = true;				barcodeInfos[i]->setColBinRegion(&bin);
				UA_DOUT(4, 9, "overlaps col " << c);
				if (left < min) {
					bin.setMin(left);
					UA_DOUT(4, 9, "col " << c << " update min " << bin.getMin());
				}
				if (left > max) {
					bin.setMax(left);
					UA_DOUT(4, 9, "col " << c << " update max " << bin.getMax());
				}
				if (right < min) {
					bin.setMin(right);
					UA_DOUT(4, 9, "col " << c << " update min " << bin.getMin());
				}
				if (right > max) {
					bin.setMax(right);
					UA_DOUT(4, 9, "col " << c << " update max " << bin.getMax());
				}
				break;
			}
		}

		for (unsigned r = 0, rn = rowBinRegions.size(); r < rn; ++r) {
			BinRegion & bin = *rowBinRegions[r];

			unsigned min = bin.getMin();
			unsigned max = bin.getMax();
			unsigned top = static_cast<unsigned>(tlCorner.Y);
			unsigned bottom = static_cast<unsigned>(brCorner.Y);

			if (((min - BIN_THRESH <= top) && (top <= max + BIN_THRESH))
					|| ((min - BIN_THRESH <= bottom) && (bottom <= max + BIN_THRESH))) {
				insideRowBin = true;
				barcodeInfos[i]->setRowBinRegion(&bin);
				UA_DOUT(4, 9, "overlaps row " << r);
				if (top < min) {
					bin.setMin(top);
					UA_DOUT(4, 9, "row " << r << " update min " << bin.getMin());
				}
				if (top > max) {
					bin.setMax(top);
					UA_DOUT(4, 9, "row " << r << " update max " << bin.getMax());
				}
				if (bottom < min) {
					bin.setMin(bottom);
					UA_DOUT(4, 9, "row " << r << " update min " << bin.getMin());
				}
				if (bottom > max) {
					bin.setMax(bottom);
					UA_DOUT(4, 9, "row " << r << " update max " << bin.getMax());
				}
				break;
			}
		}

		if (!insideColBin) {
			BinRegion * newBinRegion = new BinRegion(
					BinRegion::ORIENTATION_VER,
					static_cast<unsigned> (tlCorner.X),
					static_cast<unsigned> (brCorner.X));
			UA_ASSERT_NOT_NULL(newBinRegion);
			UA_DOUT(4, 9, "new col " << colBinRegions.size() << ": " << *newBinRegion);
			colBinRegions.push_back(newBinRegion);
			barcodeInfos[i]->setColBinRegion(newBinRegion);
		}

		if (!insideRowBin) {
			BinRegion * newBinRegion = new BinRegion(
					BinRegion::ORIENTATION_HOR,
					static_cast<unsigned> (tlCorner.Y),
					static_cast<unsigned> (brCorner.Y));
			UA_ASSERT_NOT_NULL(newBinRegion);
			UA_DOUT(4, 9, "new row " << rowBinRegions.size() << ": " << *newBinRegion);
			rowBinRegions.push_back(newBinRegion);
			barcodeInfos[i]->setRowBinRegion(newBinRegion);
		}

		std::ostringstream msg;
		for (unsigned c = 0, n = colBinRegions.size(); c < n; ++c) {
			BinRegion & region = *colBinRegions[c];
			msg << c << " (" << region.getMin() << ", " << region.getMax()
					<< "), ";
		}
		UA_DOUT(4, 9, "columns " << msg.str());

		msg.str("");
		for (unsigned r = 0, rn = rowBinRegions.size(); r < rn; ++r) {
			BinRegion & region = *rowBinRegions[r];
			msg << r << " (" << region.getMin() << ", " << region.getMax()
					<< "), ";
		}
		UA_DOUT(4, 9, "rows " << msg.str());
	}

	sort(rowBinRegions.begin(), rowBinRegions.end(), BinRegionSort());
	sort(colBinRegions.begin(), colBinRegions.end(), BinRegionSort());

	// assign ranks now and add threshold
	for (unsigned i = 0, n = colBinRegions.size(); i < n; ++i) {
		BinRegion & c = *colBinRegions[i];

		unsigned min = c.getMin();
		c.setMin(min > BIN_MARGIN ? min - BIN_MARGIN : 0);

		unsigned max = c.getMax();
		c.setMax(max < width - BIN_MARGIN - 1 ? max + BIN_MARGIN : width - 1);

		c.setRank(i);
		UA_DOUT(4, 5, "col BinRegion " << i << ": " << c);
	}
	for (unsigned i = 0, n = rowBinRegions.size(); i < n; ++i) {
		BinRegion & c = *rowBinRegions[i];

		unsigned min = c.getMin();
		c.setMin(min > BIN_MARGIN ? min - BIN_MARGIN : 0);

		unsigned max = c.getMax();
		c.setMax(max < height - BIN_MARGIN - 1 ? max + BIN_MARGIN : height - 1);

		c.setRank(i);
		UA_DOUT(4, 5, "row BinRegion " << i << ": " << c);
	}

	UA_DOUT(4, 3, "number of columns: " << colBinRegions.size());
	UA_DOUT(4, 3, "number of rows: " << rowBinRegions.size());

	sort(barcodeInfos.begin(), barcodeInfos.end(), BarcodeInfoSort());
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

	dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords,
			reg->sizeIdx);

	rotate = (2 * M_PI) + (atan2(reg->fit2raw[0][1], reg->fit2raw[1][1])
			- atan2(reg->fit2raw[1][0], reg->fit2raw[0][0])) / 2.0;

	rotateInt = (int) (rotate * 180 / M_PI + 0.5);
	if (rotateInt >= 360)
		rotateInt -= 360;

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

/*
 *	createDmtxImageFromDib
 *
 */
DmtxImage * Decoder::createDmtxImageFromDib(Dib & dib) {
	int pack = DmtxPackCustom;

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

	// create dmtxImage from the dib
	DmtxImage * image = dmtxImageCreate(dib.getPixelBuffer(), dib.getWidth(),
			dib.getHeight(), pack);

	//set the properties (pad bytes, flip)
	dmtxImageSetProp(image, DmtxPropRowPadBytes, dib.getRowPadBytes());
	dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
	return image;
}

void Decoder::imageShowBarcodes(Dib & dib) {
	UA_DOUT(4, 3, "marking tags ");

	RgbQuad quadGreen(0, 255, 0);

	for (unsigned i = 0, n = barcodeInfos.size(); i < n; ++i) {
		BarcodeInfo & info = *barcodeInfos[i];
		DmtxPixelLoc & tlCorner = info.getTopLeftCorner();
		DmtxPixelLoc & brCorner = info.getBotRightCorner();

		dib.line(tlCorner.X, tlCorner.Y, tlCorner.X, brCorner.Y, quadGreen);
		dib.line(tlCorner.X, brCorner.Y, brCorner.X, brCorner.Y, quadGreen);
		dib.line(brCorner.X, brCorner.Y, brCorner.X, tlCorner.Y, quadGreen);
		dib.line(brCorner.X, tlCorner.Y, tlCorner.X, tlCorner.Y, quadGreen);
	}

	unsigned height = dib.getHeight() - 1;
	unsigned width = dib.getWidth() - 1;

	for (unsigned r = 0, rn = rowBinRegions.size(); r < rn; ++r) {
		BinRegion & region = *rowBinRegions[r];

		unsigned min = region.getMin();
		unsigned max = region.getMax();

		dib.line(0, min, width, min, quadGreen);
		dib.line(0, min, 0, max, quadGreen);
		dib.line(0, max, width, max, quadGreen);
		dib.line(width, min, width, max, quadGreen);
	}

	for (unsigned c = 0, n = colBinRegions.size(); c < n; ++c) {
		BinRegion & region = *colBinRegions[c];

		unsigned min = region.getMin();
		unsigned max = region.getMax();

		dib.line(min, 0, max, 0, quadGreen);
		dib.line(min, height, max, height, quadGreen);
		dib.line(min, 0, min, height, quadGreen);
		dib.line(max, 0, max, height, quadGreen);
	}
}

ostream & operator<<(ostream &os, DecodeRegion & r) {
	os << r.row << "," << r.col << ": " << "(" << r.topLeft.X << ", "
			<< r.topLeft.Y << "), " << "(" << r.botRight.X << ", "
			<< r.botRight.Y << ")";
	return os;
}
