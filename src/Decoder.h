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
#include "cvblob/include/BlobResult.h"

#include <vector>
#include <string>

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

	ProcessResult processImageRegions(unsigned plateNum, Dib & dib,
			vector<vector<string> > & cells);
	
	ProcessResult superProcessImageRegions(Dib & dib,IplImage *opencvImg,vector<vector<string> > & cells, bool matrical);

	void imageShowBarcodes(Dib & dib, bool regions);

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
	vector<BinRegion *>   rowBinRegions;
	vector<BinRegion *>   colBinRegions;
	vector<vector<string> > cells;

	

	unsigned char * imageBuf;

	void clearResults();
	void messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	DmtxImage * createDmtxImageFromDib(Dib & dib);
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	bool processImage(Dib & dib, CvRect croppedOffset);
	void calcRowsAndColumns();
	ProcessResult calculateSlots(double dpi);
	void initCells(unsigned maxRow, unsigned maxCol);
	bool decode(DmtxDecode *& dec, unsigned attempts,
			vector<BarcodeInfo *> & barcodeInfos, CvRect croppedOffset);
};

#endif /* DECODER_H_ */
