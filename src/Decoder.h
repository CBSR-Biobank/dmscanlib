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
};

ostream & operator<<(ostream &os, DecodeRegion & r);

class Decoder {
public:
	Decoder();
	Decoder(Dib & dib);
	Decoder(DmtxImage & image);
	virtual ~Decoder();

	void processDib(Dib & dib);
	void processImageRegions(Dib & dib);
	virtual void processImage(DmtxImage & image);

	unsigned getNumTags();
	const char * getTag(unsigned tagNum);
	void getTagBoundingBox(int tagNum, DmtxVector2 & p00, DmtxVector2 & p10,
			DmtxVector2 & p11, DmtxVector2 & p01);

	string getResults();
	void debugShowTags();
	void getRegionsFromIni(CSimpleIniA & ini);

protected:

	static const char * INI_SECTION_NAME;
	static const char * INI_REGION_LABEL;

	vector<MessageInfo *>  calRegions;
	vector<DecodeRegion *> decodeRegions;

	void clearResults();
	void messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	DmtxImage * createDmtxImageFromDib(Dib & dib);
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
};

#endif /* DECODER_H_ */
