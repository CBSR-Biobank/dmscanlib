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
class MessageInfo;

struct DecodeRegion {
	int row, col;
	DmtxPixelLoc topLeft, botRight;
	MessageInfo * msgInfo;

	DecodeRegion() {
		msgInfo = NULL;
	}
};

ostream & operator<<(ostream &os, DecodeRegion & r);

class Decoder {
public:
	Decoder();
	virtual ~Decoder();

	void processImageRegions(CSimpleIniA & ini, Dib & dib);
	vector<DecodeRegion *> & getDecodeRegions() {
		return decodeRegions;
	}

protected:

	static const char * INI_SECTION_NAME;
	static const char * INI_REGION_LABEL;

	vector<DecodeRegion *> decodeRegions;

	void clearResults();
	void getRegionsFromIni(CSimpleIniA & ini);
	void messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	DmtxImage * createDmtxImageFromDib(Dib & dib);
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	void processImage(Dib & dib, vector<MessageInfo *>  & msgInfos);
	void processImage(DmtxImage & image, vector<MessageInfo *>  & msgInfos);
};

#endif /* DECODER_H_ */
