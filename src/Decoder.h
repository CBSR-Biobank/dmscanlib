#ifndef DECODER_H_
#define DECODER_H_

/*
 * Decoder.h
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "dmtx.h"

#include <vector>
#include <string>

using namespace std;

class Dib;
struct RgbQuad;
class BarcodeInfo;
class BinRegion;

class Decoder {
public:
	Decoder(unsigned scanGap, unsigned squareDev, unsigned edgeThresh);
	virtual ~Decoder();

	bool processImageRegions(unsigned plateNum, Dib & dib, string & msg);
	void imageShowBarcodes(Dib & dib);

protected:
	bool decode(DmtxDecode *& dec, unsigned attempts,
			vector<BarcodeInfo *> & barcodeInfos);

	static const char * INI_SECTION_NAME;
	static const char * INI_REGION_LABEL;
	static const unsigned BIN_THRESH = 15;
	static const unsigned BIN_MARGIN = 15;
	static const double SLOT_DISTANCE = 0.3; // inches between slots

	unsigned scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	vector<BarcodeInfo *> barcodeInfos;
	vector<BinRegion *>   rowBinRegions;
	vector<BinRegion *>   colBinRegions;
	unsigned width;
	unsigned height;

	void clearResults();
	void messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	DmtxImage * createDmtxImageFromDib(Dib & dib);
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	bool processImage(Dib & dib);
	void calcRowsAndColumns();
	void calculateSlots(double dpi);
	void getDecodeLoacations(unsigned plateNum, string & msg);
};

#endif /* DECODER_H_ */
