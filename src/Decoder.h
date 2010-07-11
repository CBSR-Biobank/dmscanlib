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

	ProcessResult processImageRegionsDmtx(unsigned plateNum, Dib & dib);
	ProcessResult processImageRegionsCv(Dib & dib,
			IplImage *opencvImg, bool metrical);
	ProcessResult processImageRegionsCvThreaded(Dib * dib,
			IplImage *opencvImg, bool metrical);

	void imageShowBarcodes(Dib & dib, bool regions);
	vector<vector<string> > & getCells() { return cells; }

	static DmtxImage * createDmtxImageFromDib(Dib & dib);


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
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	bool processImage(Dib & dib, CvRect croppedOffset);
	void calcRowsAndColumns();
	ProcessResult calculateSlots(double dpi);
	void initCells(unsigned maxRow, unsigned maxCol);
	bool decode(DmtxDecode *& dec, unsigned attempts,
			vector<BarcodeInfo *> & barcodeInfos, CvRect croppedOffset);
};

/*----threading----*/

void processImageThreaded(void * param);
/*----threading----*/


#endif /* DECODER_H_ */
