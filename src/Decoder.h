#ifndef DECODER_H_
#define DECODER_H_

/*
 * Decoder.h
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "dmtx.h"
#include "cv.h"

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
			unsigned corrections, double cellDistance);
	virtual ~Decoder();

	typedef enum {
		IMG_INVALID,
		POS_INVALID,
		POS_CALC_ERROR,
		OK
	} ProcessResult;

	ProcessResult processImageRegions(Dib * dib,
			IplImage *opencvImg, bool metrical);

	void imageShowBarcodes(Dib & dib, bool regions);
	vector<vector<string> > & getCells() { return cells; }

	static DmtxImage * createDmtxImageFromDib(Dib & dib);

	/**
	 * Called by subordinates to add a barcode. Returns NULL if the barcode has
	 * previously been added.
	 *
	 * @param dec Pointer to the libdmtx decode structure.
	 * @param reg Pointer to the libdmtx region structure.
	 * @param msg Pointer to the libdmtx message structure.
	 *
	 * @return Pointer to the barcode object that was added or NULL if the
	 * barcode has already been added.
	 */
	BarcodeInfo * addBarcodeInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);


protected:

	static const char * INI_SECTION_NAME;
	static const char * INI_REGION_LABEL;
	static const unsigned BIN_THRESH = 15;
	static const unsigned BIN_MARGIN = 15;

	double scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	unsigned corrections;
	double cellDistance;
	unsigned width;
	unsigned height;
	unsigned dpi;
	vector<BarcodeInfo *> barcodeInfos;
	map<string, BarcodeInfo *> barcodesMap;
	vector<BinRegion *>   rowBinRegions;
	vector<BinRegion *>   colBinRegions;
	vector<vector<string> > cells;

	unsigned char * imageBuf;
	OpenThreads::Mutex addBarcodeMutex;


	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	void calcRowsAndColumns();
	ProcessResult calculateSlots(double dpi);
	void initCells(unsigned maxRow, unsigned maxCol);
	static void getTubeBlobsFromDpi(vector < CvRect > &blobVector,
			IplImage * opencvImg, bool metrical, int dpi);
	static void getTubeBlobs(IplImage * original, int threshold,
			int blobsize, int blurRounds, int border,
			vector <CvRect> & blobVector);
};


#endif /* DECODER_H_ */
