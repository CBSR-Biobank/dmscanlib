/*
 * Calibrator.cpp
 *
 *  Created on: 5-Jun-2009
 *      Author: loyola
 */

#include "Calibrator.h"
#include "MessageInfo.h"
#include "BinRegion.h"
#include "UaDebug.h"

Calibrator::Calibrator() {
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

void Calibrator::processImage(DmtxImage & image) {
	Decoder::processImage(image);
	unsigned width = dmtxImageGetProp(&image, DmtxPropWidth);
	unsigned height = dmtxImageGetProp(&image, DmtxPropHeight);
	sortRegions(height, width);
}

/* Finds rows and columns by examining each decode region's top left corner.
 * Each region is assigned to a row and column.
 *
 * Once rows and columns are determined, they are sorted. Once this is done,
 * the regions can then be sorted according to row and column.
 */
void Calibrator::sortRegions(unsigned imageHeight, unsigned imageWidth) {
	bool insideRowBin;
	bool insideColBin;

	for (unsigned i = 0, n = calRegions.size(); i < n; ++i) {
		insideRowBin = false;
		insideColBin = false;

		DmtxVector2 & tlCorner = calRegions[i]->getTopLeftCorner();
		DmtxVector2 & brCorner = calRegions[i]->getBotRightCorner();

		UA_DOUT(1, 9, "tag " << i << " : corner/" << tlCorner.X << "," << tlCorner.Y);

		for (unsigned c = 0, cn = colBinRegions.size(); c < cn; ++c) {
			BinRegion & bin = *colBinRegions[c];

			int lDiff = (int) tlCorner.X - bin.getMin();
			int rDiff = (int) brCorner.X - bin.getMax();

			UA_DOUT(1, 9, "col " << c << ": left_diff/" << lDiff << ": right_diff/" << rDiff);

			if ((lDiff >= 0) && (rDiff <= 0)) {
				insideColBin = true;
			}
			else if ((lDiff < 0) && (lDiff > -BIN_THRESH)) {
				insideColBin = true;
				bin.setMin((int) tlCorner.X);
				UA_DOUT(1, 9, "col update min " << bin.getMin());
			}
			else if ((rDiff > 0) && (rDiff < BIN_THRESH)) {
				insideColBin = true;
				bin.setMax((int) brCorner.X);
				UA_DOUT(1, 9, "col update max " << bin.getMax());
			}
		}

		for (unsigned r = 0, rn = rowBinRegions.size(); r < rn; ++r) {
			BinRegion & bin = *rowBinRegions[r];

			int tDiff = (int) tlCorner.Y - bin.getMin();
			int bDiff = (int) brCorner.Y - bin.getMax();

			UA_DOUT(1, 9, "row " << r << ": top_diff/" << tDiff << ": bot_diff/" << bDiff);

			if ((tDiff >= 0) && (bDiff <= 0)) {
				insideRowBin = true;
			}
			else if ((tDiff < 0) && (tDiff > -BIN_THRESH)) {
				insideRowBin = true;
				bin.setMin((int) tlCorner.Y);
				UA_DOUT(1, 9, "row update min " << bin.getMin());
			}
			else if ((bDiff > 0) && (bDiff < BIN_THRESH)) {
				insideRowBin = true;
				bin.setMax((int) brCorner.Y);
				UA_DOUT(1, 9, "row update max " << bin.getMax());
			}
		}

		if (!insideColBin) {
			BinRegion * newBinRegion = new BinRegion(BinRegion::ORIENTATION_VER,
					(unsigned) tlCorner.X, (unsigned) brCorner.X);
			UA_ASSERT_NOT_NULL(newBinRegion);
			UA_DOUT(1, 9, "new col " << colBinRegions.size() << ": " << *newBinRegion);
			colBinRegions.push_back(newBinRegion);
			calRegions[i]->setColBinRegion(newBinRegion);
		}

		if (!insideRowBin) {
			BinRegion * newBinRegion = new BinRegion(BinRegion::ORIENTATION_HOR,
					(unsigned) tlCorner.Y, (unsigned) brCorner.Y);
			UA_ASSERT_NOT_NULL(newBinRegion);
			UA_DOUT(1, 9, "new row " << rowBinRegions.size() << ": " << *newBinRegion);
			rowBinRegions.push_back(newBinRegion);
			calRegions[i]->setRowBinRegion(newBinRegion);
		}
	}

	sort(rowBinRegions.begin(), rowBinRegions.end(), BinRegionSort());
	sort(colBinRegions.begin(), colBinRegions.end(), BinRegionSort());

	// assign ranks now
	for (unsigned i = 0, n = colBinRegions.size(); i < n; ++ i) {
		BinRegion & c = *colBinRegions[i];
		c.setRank(i);
		UA_DOUT(1, 5, "col BinRegion " << i << ": " << c);
	}
	for (unsigned i = 0, n = rowBinRegions.size(); i < n; ++ i) {
		BinRegion & c = *rowBinRegions[i];
		c.setRank(i);
		UA_DOUT(1, 5, "row BinRegion " << i << ": " << c);
	}


	UA_DOUT(1, 3, "number of columns: " << colBinRegions.size());
	UA_DOUT(1, 3, "number of rows: " << rowBinRegions.size());

	sort(calRegions.begin(), calRegions.end(), MessageInfoSort());
}

void Calibrator::saveRegionsToIni(CSimpleIniA & ini) {
	UA_ASSERT(calRegions.size() > 0);
	SI_Error rc = ini.SetValue(INI_SECTION_NAME, NULL, NULL);
	UA_ASSERT(rc >= 0);

	unsigned maxCol = calRegions[0]->getColBinRegion().getRank();
	ostringstream key, value;
	for (unsigned i = 0, numTags = calRegions.size(); i < numTags; ++i) {
		MessageInfo & info = *calRegions[i];
		key.str("");
		value.str("");

		DmtxVector2 & tl = info.getTopLeftCorner();
		DmtxVector2 & br = info.getBotRightCorner();

		key << INI_REGION_LABEL << info.getRowBinRegion().getRank() << "_"
			<< maxCol - info.getColBinRegion().getRank();
		value << (int) tl.X << "," << (int) tl.Y << ","
			  << (int) br.X << "," << (int) br.Y;

		SI_Error rc = ini.SetValue(INI_SECTION_NAME, key.str().c_str(), value.str().c_str());
		UA_ASSERT(rc >= 0);
	}
}
