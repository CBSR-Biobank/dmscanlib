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
	BarcodeInfo * msgInfo;

	DecodeRegion() {
		msgInfo = NULL;
	}
};

ostream & operator<<(ostream &os, DecodeRegion & r);

class Decoder {
public:
	Decoder();
	virtual ~Decoder();

	void processImageRegions(unsigned plateNum, CSimpleIniA & ini, Dib & dib);
	vector<DecodeRegion *> & getDecodeRegions() {
		return decodeRegions;
	}
	void imageShowRegions(Dib & dib, RgbQuad & quad);

protected:

	static const char * INI_SECTION_NAME;
	static const char * INI_REGION_LABEL;

	vector<DecodeRegion *> decodeRegions;

	void clearResults();
	bool getRegionsFromIni(unsigned plateNum, CSimpleIniA & ini);
	void messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	DmtxImage * createDmtxImageFromDib(Dib & dib);
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	void processImage(Dib & dib, vector<BarcodeInfo *>  & msgInfos);
	void processImage(DmtxImage & image, vector<BarcodeInfo *>  & msgInfos);
};

#endif /* DECODER_H_ */
