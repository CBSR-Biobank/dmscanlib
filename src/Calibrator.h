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
	virtual void processImage(DmtxImage & image);
	void saveRegionsToIni(CSimpleIniA & ini);
	void imageShowBins(Dib & dib, RgbQuad & quad);

private:
	static const int BIN_THRESH = 15;

	vector<BinRegion *> rowBinRegions;
	vector<BinRegion *> colBinRegions;

	void sortRegions(unsigned imageHeight, unsigned imageWidth);
};

#endif /* CALIBRATOR_H_ */
