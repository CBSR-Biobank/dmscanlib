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

/*
 * Decoder.cpp
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

#include "Decoder.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Dib.h"
#include "Util.h"
#include "BarcodeInfo.h"
#include "BinRegion.h"
#include "ProcessImageManager.h"

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


Decoder::Decoder(double g, unsigned s, unsigned t, unsigned c, double dist)
{
	ua::Logger::Instance().subSysHeaderSet(3, "Decoder");
	scanGap = g;
	squareDev = s;
	edgeThresh = t;
	corrections = c;
	imageBuf = NULL;
	cellDistance = dist;
}

Decoder::~Decoder()
{
	BarcodeInfo *b;
	BinRegion *c;

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

	if (imageBuf != NULL) {
		free(imageBuf);
	}
}

void Decoder::initCells(unsigned maxRows, unsigned maxCols)
{
	cells.resize(maxRows);
	for (unsigned row = 0; row < maxRows; ++row) {
		cells[row].resize(maxCols);
	}
}


void Decoder::getTubeBlobs(Dib * dib, int threshold, int blobsize,
		int blurRounds, int border, vector <CvRect> & blobVector)
{

	/*--- generate ipl equiv ---*/
	IplImageContainer *iplFilteredDib;
	iplFilteredDib = dib->generateIplImage();
	UA_ASSERT_NOT_NULL(iplFilteredDib);
	UA_DOUT(1, 7, "generated IplImage from filteredDib");

	IplImage *original; 
	original = iplFilteredDib->getIplImage();

		/* special case: blobsize = 0 then make the whole image a blob */
	if(blobsize == 0){
		CvRect img;
		img.x = 0;
		img.y = 0;
		img.width = original->width-2;
		img.height = original->height-2;
		blobVector.clear();
		blobVector.push_back(img);
		return;
	}
	double minBlobWidth, minBlobHeight, barcodeSizeInches = 0.13;

	minBlobWidth =  ((double)iplFilteredDib->getHorizontalResolution()/39.3700787)*barcodeSizeInches;
	minBlobHeight = ((double)iplFilteredDib->getVerticalResolution()/39.3700787)*barcodeSizeInches;

	UA_DOUT(1, 8, "Minimum Blob Size (Inches): " << barcodeSizeInches);
	UA_DOUT(1, 8, "Minimum Blob Width (Pixels): " << minBlobWidth);
	UA_DOUT(1, 8, "Minimum Blob Height (Pixels): " << minBlobHeight);

	IplImage *originalThr;
	IplImage *filtered;

	CBlobResult blobs;

	filtered = cvCreateImage(cvGetSize(original), original->depth, original->nChannels);
	cvCopy(original, filtered, NULL);

	/*---filters---*/
	for (int i = 0; i < blurRounds; i++) {
		cvSmooth(filtered, filtered, CV_GAUSSIAN, 11, 11);
	}
	/*---filters---*/

	originalThr = cvCreateImage(cvGetSize(filtered), IPL_DEPTH_8U, 1);
	cvThreshold(filtered, originalThr, threshold, 255, CV_THRESH_BINARY);



	blobs = CBlobResult(originalThr, NULL, 0);
	blobs.Filter(blobs, B_EXCLUDE, CBlobGetArea(), B_LESS, blobsize);

	blobVector.clear();

	for (int i = 0; i < blobs.GetNumBlobs(); i++) {
		CvRect box = blobs.GetBlob(i)->GetBoundingBox();

		UA_ASSERTS(box.x < filtered->width
			   && box.y < filtered->height,
			   "blob is out of bounds");

		if (border < 0) {
			UA_ASSERTS(border * 2 < box.width
				   && border * 2 < box.height,
				   "cannot shrink past rect dimensions");
		}

		box.x -= border;
		box.y -= border;
		box.width += border * 2;
		box.height += border * 2;

		box.x = box.x < 0 ? 0 : box.x;
		box.y = box.y < 0 ? 0 : box.y;

		if (box.x + box.width >= filtered->width)
			box.width = (filtered->width - box.x) - 1;

		if (box.y + box.height >= filtered->height)
			box.height = (filtered->height - box.y) - 1;
	

		float contrastSum = 0, count = 0, contrastRatio = 0;

		cvSetImageROI(original, box);
		for (int i = box.y ; i < (box.y + box.height) -1; i++) {
			for (int j = box.x; j < (box.x + box.width) - 1; j++) {
				float center = (float)(original->imageData + i * original->widthStep)[j];
				float right  = (float)(original->imageData + i * original->widthStep)[j+1];
				float down  = (float)(original->imageData + (i+1) * original->widthStep)[j];

				contrastSum +=  (float)((center-right)*(center-right) + (center-down)*(center-down));
				count++;
			}
		}
		cvResetImageROI(original);

		contrastRatio = contrastSum / count;
		UA_DOUT(1, 9, "Contrast Ratio: " << contrastRatio );


		if(box.width >= minBlobWidth && box.height >= minBlobHeight && contrastRatio > 2000)
			blobVector.push_back(box);

	}
	blobs.ClearBlobs();

	cvReleaseImage(&originalThr);
	cvReleaseImage(&filtered);
	delete iplFilteredDib;
}

void Decoder::getTubeBlobsFromDpi(Dib * dib,vector < CvRect > &blobVector,
		bool metrical, int dpi)
{
	if (!metrical) {
		switch (dpi) {
		case 600:
			 getTubeBlobs(dib, 54, 2400, 5, 12, blobVector);
			break;

		case 400:
			getTubeBlobs(dib, 50, 1900, 3, 4, blobVector);
			break;

		case 300:
		default:
			getTubeBlobs(dib, 55, 840, 2, 3, blobVector);
			break;
		}
	} else {
		// metrical
		UA_DOUT(3, 7, "getTubeBlobsFromDpi: metrical mode");

		switch (dpi) {
		case 600:
			getTubeBlobs(dib, 120, 5000, 10, -10, blobVector);
			break;

		case 400:
			getTubeBlobs(dib, 120, 3000, 10, -5, blobVector);
			break;

		case 300:
		default:
			getTubeBlobs(dib, 110, 2000, 8, -4, blobVector);
			break;
		}
	}
}

Decoder::ProcessResult Decoder::processImageRegions(Dib * dib, bool matrical)
{

	vector < CvRect > blobVector;
	getTubeBlobsFromDpi(dib,blobVector, matrical, dib->getDpi()); //updates blobVector

	#ifdef _DEBUG
		{
			Dib blobDib(*dib);
			RgbQuad white(255,255,255);
			unsigned n,i;
			for ( i = 0, n = blobVector.size(); i < n; i++) {
				blobDib.rectangle(blobVector[i].x,blobVector[i].y,blobVector[i].width,blobVector[i].height,white);
			}
			blobDib.writeToFile("blobRegions.bmp");
		}
	#endif

	ProcessImageManager imageProcessor(this, scanGap, squareDev, edgeThresh, corrections);
	imageProcessor.generateBarcodes(dib, blobVector, barcodeInfos);

	if (barcodeInfos.empty()) {
		UA_DOUT(3, 5, "no barcodes were found.");
		return IMG_INVALID;
	}

	width = dib->getWidth();
	height = dib->getHeight();

	calcRowsAndColumns();

	return calculateSlots(static_cast < double >(dib->getDpi()));
}

DmtxImage * Decoder::createDmtxImageFromDib(Dib & dib)
{
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
	dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY);	// DIBs are flipped in Y
	return image;
}

void Decoder::calcRowsAndColumns()
{
	UA_ASSERTS(barcodeInfos.size() != 0,
		   "no barcodes in barcodeInfos vector");

	bool insideRowBin;
	bool insideColBin;

	for (unsigned i = 0, n = barcodeInfos.size(); i < n; ++i) {
		insideRowBin = false;
		insideColBin = false;

		DmtxPixelLoc & tlCorner = barcodeInfos[i]->getTopLeftCorner();
		DmtxPixelLoc & brCorner = barcodeInfos[i]->getBotRightCorner();

		UA_DOUT(3, 9,
			"tag " << i << " : tlCorner/" << tlCorner.X << "," <<
			tlCorner.
			Y << "  brCorner/" << brCorner.X << "," << brCorner.Y);

		for (unsigned c = 0, cn = colBinRegions.size(); c < cn; ++c) {
			BinRegion & bin = *colBinRegions[c];

			unsigned min = bin.getMin();
			unsigned max = bin.getMax();
			unsigned left = static_cast < unsigned >(tlCorner.X);
			unsigned right = static_cast < unsigned >(brCorner.X);

			if (((min - BIN_THRESH <= left)
			     && (left <= max + BIN_THRESH))
			    || ((min - BIN_THRESH <= right)
				&& (right <= max + BIN_THRESH))) {
				insideColBin = true;
				barcodeInfos[i]->setColBinRegion(&bin);
				UA_DOUT(3, 9, "overlaps col " << c);
				if (left < min) {
					bin.setMin(left);
					UA_DOUT(3, 9,
						"col " << c << " update min " <<
						bin.getMin());
				}
				if (left > max) {
					bin.setMax(left);
					UA_DOUT(3, 9,
						"col " << c << " update max " <<
						bin.getMax());
				}
				if (right < min) {
					bin.setMin(right);
					UA_DOUT(3, 9,
						"col " << c << " update min " <<
						bin.getMin());
				}
				if (right > max) {
					bin.setMax(right);
					UA_DOUT(3, 9,
						"col " << c << " update max " <<
						bin.getMax());
				}
				break;
			}
		}

		for (unsigned r = 0, rn = rowBinRegions.size(); r < rn; ++r) {
			BinRegion & bin = *rowBinRegions[r];

			unsigned min = bin.getMin();
			unsigned max = bin.getMax();
			unsigned top = static_cast < unsigned >(tlCorner.Y);
			unsigned bottom = static_cast < unsigned >(brCorner.Y);

			if (((min - BIN_THRESH <= top)
			     && (top <= max + BIN_THRESH))
			    || ((min - BIN_THRESH <= bottom)
				&& (bottom <= max + BIN_THRESH))) {
				insideRowBin = true;
				barcodeInfos[i]->setRowBinRegion(&bin);
				UA_DOUT(3, 9, "overlaps row " << r);
				if (top < min) {
					bin.setMin(top);
					UA_DOUT(3, 9,
						"row " << r << " update min " <<
						bin.getMin());
				}
				if (top > max) {
					bin.setMax(top);
					UA_DOUT(3, 9,
						"row " << r << " update max " <<
						bin.getMax());
				}
				if (bottom < min) {
					bin.setMin(bottom);
					UA_DOUT(3, 9,
						"row " << r << " update min " <<
						bin.getMin());
				}
				if (bottom > max) {
					bin.setMax(bottom);
					UA_DOUT(3, 9,
						"row " << r << " update max " <<
						bin.getMax());
				}
				break;
			}
		}

		if (!insideColBin) {
			BinRegion *newBinRegion =
			    new BinRegion(BinRegion::ORIENTATION_VER,
					  static_cast < unsigned >(tlCorner.X),
					  static_cast < unsigned >(brCorner.X));
			UA_ASSERT_NOT_NULL(newBinRegion);
			UA_DOUT(3, 9,
				"new col " << colBinRegions.size() << ": " <<
				*newBinRegion);
			colBinRegions.push_back(newBinRegion);
			barcodeInfos[i]->setColBinRegion(newBinRegion);
		}

		if (!insideRowBin) {
			BinRegion *newBinRegion =
			    new BinRegion(BinRegion::ORIENTATION_HOR,
					  static_cast < unsigned >(tlCorner.Y),
					  static_cast < unsigned >(brCorner.Y));
			UA_ASSERT_NOT_NULL(newBinRegion);
			UA_DOUT(3, 9,
				"new row " << rowBinRegions.size() << ": " <<
				*newBinRegion);
			rowBinRegions.push_back(newBinRegion);
			barcodeInfos[i]->setRowBinRegion(newBinRegion);
		}

		ostringstream msg;
		for (unsigned c = 0, n = colBinRegions.size(); c < n; ++c) {
			BinRegion & region = *colBinRegions[c];
			msg << c << " (" << region.
			    getMin() << ", " << region.getMax()
			    << "), ";
		}
		UA_DOUT(3, 9, "columns " << msg.str());

		msg.str("");
		for (unsigned r = 0, rn = rowBinRegions.size(); r < rn; ++r) {
			BinRegion & region = *rowBinRegions[r];
			msg << r << " (" << region.
			    getMin() << ", " << region.getMax()
			    << "), ";
		}
		UA_DOUT(3, 9, "rows " << msg.str());
	}

	sort(rowBinRegions.begin(), rowBinRegions.end(), BinRegionSort());
	sort(colBinRegions.begin(), colBinRegions.end(), BinRegionSort());

	// assign ranks now and add threshold
	for (unsigned i = 0, n = colBinRegions.size(); i < n; ++i) {
		BinRegion & c = *colBinRegions[i];

		unsigned min = c.getMin();
		c.setMin(min > BIN_MARGIN ? min - BIN_MARGIN : 0);

		unsigned max = c.getMax();
		c.setMax(max <
			 width - BIN_MARGIN - 1 ? max + BIN_MARGIN : width - 1);

		c.setRank(i);
		UA_DOUT(3, 5, "col BinRegion " << i << ": " << c);
	}
	for (unsigned i = 0, n = rowBinRegions.size(); i < n; ++i) {
		BinRegion & c = *rowBinRegions[i];

		unsigned min = c.getMin();
		c.setMin(min > BIN_MARGIN ? min - BIN_MARGIN : 0);

		unsigned max = c.getMax();
		c.setMax(max <
			 height - BIN_MARGIN - 1 ? max + BIN_MARGIN : height -
			 1);

		c.setRank(i);
		UA_DOUT(3, 5, "row BinRegion " << i << ": " << c);
	}

	UA_DOUT(3, 3, "number of columns: " << colBinRegions.size());
	UA_DOUT(3, 3, "number of rows: " << rowBinRegions.size());

	sort(barcodeInfos.begin(), barcodeInfos.end(), BarcodeInfoSort());
}

Decoder::ProcessResult Decoder::calculateSlots(double dpi)
{
	// for columns the one with largest rank is column 1
	unsigned numCols = colBinRegions.size();
	unsigned numRows = rowBinRegions.size();

	if (numCols > 0) {
		BinRegion & region = *colBinRegions[numCols - 1];

		// Calculate the distance of the 12'th column to check that it is within
		// the bounds of the image. If not then the column is not really the
		// first one.
		double edgeDist = 11.0 * cellDistance * dpi;
		UA_DOUT(3, 5, "first_col_center/" << region.getCenter()
			<< " edge_distance/" << edgeDist);
		if (region.getCenter() <= static_cast < unsigned >(edgeDist)) {
			UA_DOUT(3, 5, "out of bounds");
			return POS_CALC_ERROR;
		}

		region.setId(0);
	}

	if (numRows > 0) {
		BinRegion & region = *rowBinRegions[0];

		// Calculate the distance of the 8'th row to check that it is within
		// the bounds of the image. If not then the column is not really the
		// first one.
		double edgeDist = 7.0 * cellDistance * dpi;
		UA_DOUT(3, 5, "first_row_center/" << region.getCenter()
			<< " edge_distance/" << edgeDist << " height/" <<
			height);
		if (region.getCenter() + static_cast < unsigned >(edgeDist) >=
		    height) {
			UA_DOUT(3, 5, "out of bounds");
			return POS_CALC_ERROR;
		}
		rowBinRegions[0]->setId(0);
	}

	unsigned interval;
	double cellDistError = cellDistance * 0.4;

	UA_DOUT(3, 5, "cellDistError/" << cellDistError);

	if (numCols > 1) {
		for (int c = numCols - 1; c > 0; --c) {
			BinRegion & region1 = *colBinRegions[c - 1];
			BinRegion & region2 = *colBinRegions[c];

			double dist = static_cast < double >(region2.getCenter()
							     -
							     region1.getCenter
							     ()) / static_cast <
			    double >(dpi);

			interval = 0;
			for (unsigned i = 1; i < 12; ++i) {
				double diff = abs(dist - i * cellDistance);
				UA_DOUT(3, 8,
					"col region " << c << "-" << c -
					1 << " distance/" << dist << " diff/" <<
					diff << " inteval/" << i);
				if (diff < cellDistError) {
					interval = i;
					break;
				}
			}
			if (interval == 0) {
				UA_DOUT(3, 1,
					"could not determine column intervals");
				return POS_CALC_ERROR;
			}

			UA_DOUT(3, 8,
				"col region " << c << "-" << c -
				1 << " distance/" << dist << " inteval/" <<
				interval);

			region1.setId(region2.getId() + interval);
		}
	}

	if (numRows > 1) {
		for (unsigned r = 1; r < numRows; ++r) {
			BinRegion & region1 = *rowBinRegions[r - 1];
			BinRegion & region2 = *rowBinRegions[r];

			double dist = static_cast < double >(region2.getCenter()
							     -
							     region1.getCenter
							     ()) / static_cast <
			    double >(dpi);

			interval = 0;
			for (unsigned i = 1; i < 8; ++i) {
				double diff = abs(dist - i * cellDistance);
				UA_DOUT(3, 5,
					"row region " << r << "-" << r -
					1 << " distance/" << dist << " diff/" <<
					diff << " inteval/" << i);
				if (abs(dist - i * cellDistance) <
				    cellDistError) {
					interval = i;
					break;
				}
			}
			if (interval == 0) {
				UA_DOUT(3, 1,
					"could not determine row intervals");
				return POS_CALC_ERROR;
			}

			UA_DOUT(3, 5,
				"row region " << r << "-" << r -
				1 << " distance/" << dist << " inteval/" <<
				interval);

			region2.setId(region1.getId() + interval);
		}
	}
	// get max rows and max cols
	unsigned maxRow = 0;
	unsigned maxCol = 0;
	for (unsigned i = 0, n = barcodeInfos.size(); i < n; ++i) {
		BarcodeInfo & info = *barcodeInfos[i];
		unsigned row = info.getRowBinRegion().getId();
		unsigned col = info.getColBinRegion().getId();

		if (row > maxRow) {
			maxRow = row;
		}

		if (col > maxCol) {
			maxCol = col;
		}
	}

	if ((maxRow >= 8) || (maxCol >= 12)) {
		return POS_CALC_ERROR;
	}
	// make sure no barcodes are in the same cells
	initCells(maxRow + 1, maxCol + 1);
	for (unsigned i = 0, n = barcodeInfos.size(); i < n; ++i) {
		BarcodeInfo & info = *barcodeInfos[i];
		unsigned row = info.getRowBinRegion().getId();
		unsigned col = info.getColBinRegion().getId();

		UA_DOUT(3, 5,
			"barcode " << i << " (" << (char)('A' +
							  row) << ", " << col +
			1 << "): " << info.getMsg());

		if (cells[row][col].length() > 0) {
			UA_DOUT(3, 5, "position (" << (char)('A' + row) << ", "
				<< col + 1 << ") already occupied");
			return POS_CALC_ERROR;
		}

		cells[row][col] = info.getMsg();
	}
	return OK;
}

void Decoder::showStats(DmtxDecode * dec, DmtxRegion * reg, DmtxMessage * msg)
{
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
			       - atan2(reg->fit2raw[1][0],
				       reg->fit2raw[0][0])) / 2.0;

	rotateInt = (int)(rotate * 180 / M_PI + 0.5);
	if (rotateInt >= 360)
		rotateInt -= 360;

	fprintf(stdout, "--------------------------------------------------\n");
	fprintf(stdout, "       Matrix Size: %d x %d\n",
		dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx),
		dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx));
	fprintf(stdout, "    Data Codewords: %d (capacity %d)\n",
		dataWordLength - msg->padCount, dataWordLength);
	fprintf(stdout, "   Error Codewords: %d\n",
		dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords,
				       reg->sizeIdx));
	fprintf(stdout, "      Data Regions: %d x %d\n",
		dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions,
				       reg->sizeIdx),
		dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions,
				       reg->sizeIdx));
	fprintf(stdout, "Interleaved Blocks: %d\n",
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

void Decoder::imageShowBarcodes(Dib & dib, bool regions)
{

	UA_DOUT(3, 3, "marking tags ");
	if(barcodeInfos.size() == 0)
		return;

	RgbQuad quadWhite(255, 255, 255);	// change to white (shows up better in grayscale)
	RgbQuad quadPink(255, 0, 255);
	RgbQuad quadRed(255, 0, 0);

	RgbQuad & highlightQuad =
	    (dib.getBitsPerPixel() == 8 ? quadWhite : quadPink);

	for (unsigned i = 0, n = barcodeInfos.size(); i < n; ++i) {
		BarcodeInfo & info = *barcodeInfos[i];
		DmtxPixelLoc & tlCorner = info.getTopLeftCorner();
		DmtxPixelLoc & brCorner = info.getBotRightCorner();

		dib.line(tlCorner.X, tlCorner.Y, tlCorner.X, brCorner.Y,
			 highlightQuad);
		dib.line(tlCorner.X, brCorner.Y, brCorner.X, brCorner.Y,
			 highlightQuad);
		dib.line(brCorner.X, brCorner.Y, brCorner.X, tlCorner.Y,
			 highlightQuad);
		dib.line(brCorner.X, tlCorner.Y, tlCorner.X, tlCorner.Y,
			 highlightQuad);
	}

	unsigned logLevel = ua::Logger::Instance().levelGet(3);

	if (logLevel == 0 || !regions)
		return;

	unsigned height = dib.getHeight() - 1;
	unsigned width = dib.getWidth() - 1;

	for (unsigned r = 0, rn = rowBinRegions.size(); r < rn; ++r) {
		BinRegion & region = *rowBinRegions[r];

		unsigned min = region.getMin();
		unsigned max = region.getMax();
		//unsigned center = region.getCenter();

		dib.line(0, min, width, min, highlightQuad);
		dib.line(0, min, 0, max, highlightQuad);
		dib.line(0, max, width, max, highlightQuad);
		dib.line(width, min, width, max, highlightQuad);
		//dib.line(0, center, width, center, quadRed);
	}

	for (unsigned c = 0, n = colBinRegions.size(); c < n; ++c) {
		BinRegion & region = *colBinRegions[c];

		unsigned min = region.getMin();
		unsigned max = region.getMax();
		//unsigned center = region.getCenter();

		dib.line(min, 0, max, 0, highlightQuad);
		dib.line(min, height, max, height, highlightQuad);
		dib.line(min, 0, min, height, highlightQuad);
		dib.line(max, 0, max, height, highlightQuad);
		//dib.line(center, 0, center, height, quadRed);
	}
}

// this method needs to be thread safe
BarcodeInfo * Decoder::addBarcodeInfo(DmtxDecode *dec, DmtxRegion *reg,
		DmtxMessage *msg) {
	addBarcodeMutex.lock();
	BarcodeInfo * info = NULL;

	string str((char *)msg->output, msg->outputIdx);

	if (barcodesMap[str] == NULL) {
		// this is a unique barcode
		info = new BarcodeInfo(dec, reg, msg);
		UA_ASSERT_NOT_NULL(info);
		barcodesMap[str] = info;
		barcodeInfos.push_back(info);
		UA_DOUT(3, 9, "Succesfully added barcode: " << info->getMsg());
	}

	addBarcodeMutex.unlock();
	return info;
}
