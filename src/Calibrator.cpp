/*
 * Calibrator.cpp
 *
 *  Created on: 5-Jun-2009
 *      Author: loyola
 */

#include "Calibrator.h"
#include "BarcodeInfo.h"
#include "BinRegion.h"
#include "Dib.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "Util.h"

Calibrator::Calibrator() {
	ua::Logger::Instance().subSysHeaderSet(4, "Calibrator");
}

Calibrator::~Calibrator() {
	BinRegion * c;

	while (rowBinRegions.size() > 0) {
		c = rowBinRegions.back();
		rowBinRegions.pop_back();
		delete c;
	}
	while (colBinRegions.size() > 0) {
		c = colBinRegions.back();
		colBinRegions.pop_back();
		delete c;
	}
}

bool Calibrator::processImage(Dib & dib) {
	DmtxImage * image = createDmtxImageFromDib(dib);

	processImage(*image);
	if (barcodeInfos.size() == 0) {
		UA_DOUT(4, 1, "processImage: no barcodes found");
		return false;
	}
	width = dib.getWidth();
	height = dib.getHeight();
	sortRegions();
	dmtxImageDestroy(&image);
	return true;
}

bool Calibrator::processImage(DmtxImage & image) {
	DmtxDecode * dec = NULL;
	unsigned width = dmtxImageGetProp(&image, DmtxPropWidth);
	unsigned height = dmtxImageGetProp(&image, DmtxPropHeight);

	UA_DOUT(4, 3, "processImage: image width/" << width
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
		if (!decode(dec, barcodeInfos)) {
			break;
		}

		UA_DOUT(4, 5, "retrieved message from region " << regionCount++);
	}

	dmtxDecodeDestroy(&dec);

	if (barcodeInfos.size() == 0) {
		UA_DOUT(4, 1, "processImage: no barcodes found");
		return false;
	}
	width = dmtxImageGetProp(&image, DmtxPropWidth);
	height = dmtxImageGetProp(&image, DmtxPropHeight);
	sortRegions();
	return true;
}

/* Finds rows and columns by examining each decode region's top left corner.
 * Each region is assigned to a row and column.
 *
 * Once rows and columns are determined, they are sorted. Once this is done,
 * the regions can then be sorted according to row and column.
 */
void Calibrator::sortRegions() {
	bool insideRowBin;
	bool insideColBin;

	for (unsigned i = 0, n = barcodeInfos.size(); i < n; ++i) {
		insideRowBin = false;
		insideColBin = false;

		DmtxPixelLoc & tlCorner = barcodeInfos[i]->getTopLeftCorner();
		DmtxPixelLoc & brCorner = barcodeInfos[i]->getBotRightCorner();

		UA_DOUT(4, 9, "tag " << i << " : tlCorner/" << tlCorner.X << "," << tlCorner.Y
				<< "  brCorner/" << brCorner.X << "," << brCorner.Y);

		for (unsigned c = 0, cn = colBinRegions.size(); c < cn; ++c) {
			BinRegion & bin = *colBinRegions[c];

			int lDiff = tlCorner.X - bin.getMin();
			int rDiff = brCorner.X - bin.getMax();

			UA_DOUT(4, 9, "col " << c << ": left_diff/" << lDiff << ": right_diff/" << rDiff);

			if ((lDiff >= 0) && (rDiff <= 0)) {
				insideColBin = true;
				barcodeInfos[i]->setColBinRegion(&bin);
			}
			else if ((lDiff < 0) && (lDiff > static_cast<int>(-BIN_THRESH))) {
				insideColBin = true;
				barcodeInfos[i]->setColBinRegion(&bin);
				bin.setMin(tlCorner.X);
				UA_DOUT(4, 9, "col update min " << bin.getMin());
			}
			else if ((rDiff > 0) && (rDiff < static_cast<int>(BIN_THRESH))) {
				insideColBin = true;
				barcodeInfos[i]->setColBinRegion(&bin);
				bin.setMax(brCorner.X);
				UA_DOUT(4, 9, "col update max " << bin.getMax());
			}
		}

		for (unsigned r = 0, rn = rowBinRegions.size(); r < rn; ++r) {
			BinRegion & bin = *rowBinRegions[r];

			int tDiff = tlCorner.Y - bin.getMin();
			int bDiff = brCorner.Y - bin.getMax();

			UA_DOUT(4, 9, "row " << r << ": top_diff/" << tDiff << ": bot_diff/" << bDiff);

			if ((tDiff >= 0) && (bDiff <= 0)) {
				insideRowBin = true;
				barcodeInfos[i]->setRowBinRegion(&bin);
			}
			else if ((tDiff < 0) && (tDiff > static_cast<int>(-BIN_THRESH))) {
				insideRowBin = true;
				barcodeInfos[i]->setRowBinRegion(&bin);
				bin.setMin(tlCorner.Y);
				UA_DOUT(4, 9, "row update min " << bin.getMin());
			}
			else if ((bDiff > 0) && (bDiff < static_cast<int>(BIN_THRESH))) {
				insideRowBin = true;
				barcodeInfos[i]->setRowBinRegion(&bin);
				bin.setMax(brCorner.Y);
				UA_DOUT(4, 9, "row update max " << bin.getMax());
			}
		}

		if (!insideColBin) {
			BinRegion * newBinRegion = new BinRegion(BinRegion::ORIENTATION_VER,
					(unsigned) tlCorner.X, (unsigned) brCorner.X);
			UA_ASSERT_NOT_NULL(newBinRegion);
			UA_DOUT(4, 9, "new col " << colBinRegions.size() << ": " << *newBinRegion);
			colBinRegions.push_back(newBinRegion);
			barcodeInfos[i]->setColBinRegion(newBinRegion);
		}

		if (!insideRowBin) {
			BinRegion * newBinRegion = new BinRegion(BinRegion::ORIENTATION_HOR,
					(unsigned) tlCorner.Y, (unsigned) brCorner.Y);
			UA_ASSERT_NOT_NULL(newBinRegion);
			UA_DOUT(4, 9, "new row " << rowBinRegions.size() << ": " << *newBinRegion);
			rowBinRegions.push_back(newBinRegion);
			barcodeInfos[i]->setRowBinRegion(newBinRegion);
		}
	}

	sort(rowBinRegions.begin(), rowBinRegions.end(), BinRegionSort());
	sort(colBinRegions.begin(), colBinRegions.end(), BinRegionSort());

	// assign ranks now and add threshold
	for (unsigned i = 0, n = colBinRegions.size(); i < n; ++ i) {
		BinRegion & c = *colBinRegions[i];

		unsigned min = c.getMin();
		c.setMin(min > BIN_MARGIN ? min - BIN_MARGIN : 0);

		unsigned max = c.getMax();
		c.setMax(max < width - BIN_MARGIN ? max + BIN_MARGIN : width);

		c.setRank(i);
		UA_DOUT(4, 5, "col BinRegion " << i << ": " << c);
	}
	for (unsigned i = 0, n = rowBinRegions.size(); i < n; ++ i) {
		BinRegion & c = *rowBinRegions[i];

		unsigned min = c.getMin();
		c.setMin(min > BIN_MARGIN ? min - BIN_MARGIN : 0);

		unsigned max = c.getMax();
		c.setMax(max < height - BIN_MARGIN ? max + BIN_MARGIN : height);

		c.setRank(i);
		UA_DOUT(4, 5, "row BinRegion " << i << ": " << c);
	}

	UA_DOUT(4, 3, "number of columns: " << colBinRegions.size());
	UA_DOUT(4, 3, "number of rows: " << rowBinRegions.size());

	sort(barcodeInfos.begin(), barcodeInfos.end(), BarcodeInfoSort());
}

void Calibrator::imageShowBins(Dib & dib, RgbQuad & quad) {
	UA_DOUT(4, 3, "marking tags ");
	for (unsigned c = 0, cn = colBinRegions.size(); c < cn; ++c) {
		dib.line(colBinRegions[c]->getMin(), 0,
				 colBinRegions[c]->getMin(), dib.getHeight()-1, quad);
		dib.line(colBinRegions[c]->getMax(), 0,
				 colBinRegions[c]->getMax(), dib.getHeight()-1, quad);
	}
	for (unsigned r = 0, rn = rowBinRegions.size(); r < rn; ++r) {
		dib.line(0, rowBinRegions[r]->getMin(),
				 dib.getWidth()-1, rowBinRegions[r]->getMin(), quad);
		dib.line(0, rowBinRegions[r]->getMax(),
				 dib.getWidth()-1, rowBinRegions[r]->getMax(), quad);

	}
}

unsigned Calibrator::getMaxCol() {
	return barcodeInfos[0]->getColBinRegion().getRank();
}
