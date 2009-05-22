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

private:
	LinkList * results;

	DmtxImage * createDmtxImageFromDib(Dib * dib);

};

#endif /* DECODER_H_ */
