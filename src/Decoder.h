#ifndef DECODER_H_
#define DECODER_H_

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

#include "dmtx.h"
#include "cv.h"
#include "IplContainer.h"
#include "ProfileSettings.h"

#include <list>
#include <string>
#include <map>
#include <OpenThreads/Mutex>

#ifdef WIN32
#include <windows.h>
#endif

using namespace std;

class Dib;
struct RgbQuad;
class BarcodeInfo;
class BinRegion;

class Decoder {
public:
	Decoder(double scanGap, unsigned squareDev, unsigned edgeThresh,
			unsigned corrections, double cellDistance, double gapX,
			double gapY, unsigned profileA, unsigned profileB,
			unsigned profileC, unsigned isHorizontal);
	virtual ~Decoder();

	typedef enum {
		IMG_INVALID, POS_INVALID, POS_CALC_ERROR, OK
	} ProcessResult;

	ProcessResult processImageRegions(Dib * dib);

	void imageShowBarcodes(Dib & dib, bool regions);
	vector<vector<string> > & getCells() {
		return cells;
	}

	static DmtxImage * createDmtxImageFromDib(Dib & dib);

	/**
	 * Called by subordinates to add a barcode. Returns NULL if the barcode has
	 * previously been added.
	 */
	BarcodeInfo * addBarcodeInfo(DmtxDecode *dec, DmtxRegion *reg,
			DmtxMessage *msg);

private:

	void reduceBlobToMatrix(unsigned blobCount, Dib * dib, CvRect & blob);
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	void initCells(unsigned maxRow, unsigned maxCol);
	static void getTubeBlobsFromDpi(Dib * dib, vector<CvRect> &blobVector,
			bool metrical, int dpi);
	static void getTubeBlobs(Dib * dib, int threshold, int blobsize,
			int blurRounds, int border, vector<CvRect> & blobVector);

	static const unsigned PALLET_ROWS = 8;
	static const unsigned PALLET_COLUMNS = 12;

	double scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	unsigned corrections;
	double cellDistance;
	unsigned width;
	unsigned height;
	unsigned dpi;
	double gapX;
	double gapY;
	ProfileSettings profile;
	bool isHorizontal;

	vector<BarcodeInfo *> barcodeInfos;
	map<string, BarcodeInfo *> barcodesMap;
	vector<vector<string> > cells;

	OpenThreads::Mutex addBarcodeMutex;
};

#endif /* DECODER_H_ */
