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

	bool processImageRegions(unsigned plateNum, Dib & dib,
			const vector<DecodeRegion *> & decodeRegions);
	void imageShowRegions(Dib & dib, const vector<DecodeRegion *> & decodeRegions);

protected:
	bool decode(DmtxDecode *& dec, unsigned attempts,
			vector<BarcodeInfo *> & barcodeInfos);

	static const char * INI_SECTION_NAME;
	static const char * INI_REGION_LABEL;

	unsigned scanGap;
	unsigned squareDev;
	unsigned edgeThresh;

	void clearResults();
	bool getRegionsFromIni(unsigned plateNum, CSimpleIniA & ini);
	void messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	DmtxImage * createDmtxImageFromDib(Dib & dib);
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	void processImage(Dib & dib, vector<BarcodeInfo *>  & barcodeInfos);
	void findSingleBarcode(DmtxImage & image, vector<BarcodeInfo *>  & barcodeInfos);
};

#endif /* DECODER_H_ */
