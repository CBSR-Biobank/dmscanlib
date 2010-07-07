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
#include <process.h>
#endif

#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Thread>

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

	ProcessResult processImageRegionsDmtx(unsigned plateNum, Dib & dib,vector<vector<string> > & cells);
	ProcessResult processImageRegionsCv(Dib & dib,IplImage *opencvImg,vector<vector<string> > & cells, bool matrical);
	ProcessResult processImageRegionsCvThreaded(Dib * dib,IplImage *opencvImg,vector<vector<string> > & cells, bool matrical);

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
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	bool processImage(Dib & dib, CvRect croppedOffset);
	void calcRowsAndColumns();
	ProcessResult calculateSlots(double dpi);
	void initCells(unsigned maxRow, unsigned maxCol);
	bool decode(DmtxDecode *& dec, unsigned attempts,
			vector<BarcodeInfo *> & barcodeInfos, CvRect croppedOffset);
};

/*----threading----*/

struct processImageParams{
	BarcodeInfo ** barcodeInfo;
	unsigned * barcodeInfoIt;
	OpenThreads::Mutex * hBarcodeInfoMutex;
	Dib * dib;
	
	CvRect croppedOffset;
	double scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	unsigned corrections;
};
void processImageThreaded(void * param);
DmtxImage * createDmtxImageFromDib(Dib & dib);
/*----threading----*/


#endif /* DECODER_H_ */
