#ifndef DECODER_H_
#define DECODER_H_

/*
 * Decoder.h
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "dmtx.h"

#define SI_SUPPORT_IOSTREAMS
#include "SimpleIni.h"

#include <vector>
#include <string>

using namespace std;

class Dib;
struct RgbQuad;
class BarcodeInfo;
class BinRegion;

struct DecodeRegion {
	int row, col;
	DmtxPixelLoc topLeft, botRight;
	BarcodeInfo * barcodeInfo;

	DecodeRegion() {
		barcodeInfo = NULL;
	}
};

ostream & operator<<(ostream &os, DecodeRegion & r);

class Decoder {
public:
	Decoder(unsigned scanGap, unsigned squareDev, unsigned edgeThresh);
	virtual ~Decoder();

	bool processImageRegions(unsigned plateNum, Dib & dib);
	void imageShowBarcodes(Dib & dib);

protected:
	bool decode(DmtxDecode *& dec, unsigned attempts,
			vector<BarcodeInfo *> & barcodeInfos);

	static const char * INI_SECTION_NAME;
	static const char * INI_REGION_LABEL;
	static const unsigned BIN_THRESH = 15;
	static const unsigned BIN_MARGIN = 15;

	unsigned scanGap;
	unsigned squareDev;
	unsigned edgeThresh;
	vector<BarcodeInfo *> barcodeInfos;
	vector<BinRegion *>   rowBinRegions;
	vector<BinRegion *>   colBinRegions;
	unsigned width;
	unsigned height;

	void clearResults();
	bool getRegionsFromIni(unsigned plateNum, CSimpleIniA & ini);
	void messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	DmtxImage * createDmtxImageFromDib(Dib & dib);
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	bool processImage(Dib & dib);
	void sortRegions();
};

#endif /* DECODER_H_ */
