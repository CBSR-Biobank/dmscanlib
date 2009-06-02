#ifndef DECODER_H_
#define DECODER_H_

/*
 * Decoder.h
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "dmtx.h"

class Dib;
class LinkList;

class Decoder {
public:
	Decoder(Dib * dib);
	Decoder(DmtxImage * image);
	virtual ~Decoder();

	void decodeImage(Dib * dib);
	void decodeImage(DmtxImage * image);

	unsigned getNumTags();
	char * getTag(int tagNum);

	void debugShowTags();

private:
	LinkList * results;
	static const unsigned NUM_SCANS = 3;

	void messageAdd(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	void getMsgRegion(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	DmtxImage * createDmtxImageFromDib(Dib * dib);
	void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);

};

#endif /* DECODER_H_ */
