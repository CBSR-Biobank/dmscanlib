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
	bool processImage(Dib & dib);
	void saveRegionsToIni(unsigned plateNum, CSimpleIniA & ini);
	void imageShowBins(Dib & dib, RgbQuad & quad);

    const vector<BarcodeInfo*> & getBarcodeInfos() const {
        return barcodeInfos;
    }

    const vector<BinRegion*> & getRowBinRegions() const {
        return rowBinRegions;
    }

    const vector<BinRegion*> & getColBinRegions() const {
        return colBinRegions;
    }

    unsigned getMaxCol();


private:
	bool processImage(DmtxImage & image);

	static const unsigned BIN_THRESH = 15;
	static const unsigned BIN_MARGIN = 12;

	unsigned width;
	unsigned height;

	vector<BarcodeInfo *> barcodeInfos;
	vector<BinRegion *>   rowBinRegions;
	vector<BinRegion *>   colBinRegions;

	void sortRegions();
};

#endif /* CALIBRATOR_H_ */
