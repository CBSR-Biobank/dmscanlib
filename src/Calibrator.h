#ifndef CALIBRATOR_H_
#define CALIBRATOR_H_

/*
 * Calibrator.h
 *
 *  Created on: 5-Jun-2009
 *      Author: loyola
 */

#include "Decoder.h"

class Dib;
class BinRegion;
struct RgbQuad;

class Calibrator : public Decoder {
public:
	Calibrator();
	virtual ~Calibrator();
	void processImage(Dib & dib);
	void processImage(DmtxImage & image);
	void saveRegionsToIni(CSimpleIniA & ini);
	void imageShowBins(Dib & dib, RgbQuad & quad);

private:
	static const int BIN_THRESH = 15;

	unsigned width;;
	unsigned height;

	vector<MessageInfo *>  msgInfos;
	vector<BinRegion *> rowBinRegions;
	vector<BinRegion *> colBinRegions;

	void sortRegions();
};

#endif /* CALIBRATOR_H_ */
