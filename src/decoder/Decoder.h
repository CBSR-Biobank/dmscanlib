#ifndef DECODER_H_
#define DECODER_H_

/*
 * Dmscanlib is a software library and standalone application that scans
 * and decodes libdmtx compatible test-tubes. It is currently designed
 * to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 * Copyright (C) 2010 Canadian Biosample Repository
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "WellRectangle.h"


#include <dmtx.h>
#include <string>
#include <vector>
#include <memory>

#ifdef WIN32
#include <windows.h>
#endif

using namespace std;

class Dib;
class DecodeOptions;
class WellDecoder;

class Decoder {
public:
    Decoder(const Dib & image, const DecodeOptions & decodeOptions,
    		vector<unique_ptr<WellRectangle<double>  > > & wellRects);
    virtual ~Decoder();
    int decodeWellRects();
    void decodeWellRect(const Dib & wellRectImage, WellDecoder & wellDecoder) const;

    const Dib & getWorkingImage() const {
    	return *filteredImage;
    }

    const unsigned getDecodedWellCount() {
    	return decodedWellCount;
    }

    const vector<WellDecoder *> & getDecodedWells() const {
    	return decodedWells;
    }

private:
    void applyFilters();
    static DmtxImage * createDmtxImageFromDib(const Dib & dib);
    void getDecodeInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg,
    		WellDecoder & wellDecoder) const;

    void writeDiagnosticImage(DmtxDecode *dec, const string & id) const;

    void showStats(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) const;

    const Dib & image;
    unique_ptr<Dib> filteredImage;
    const DecodeOptions & decodeOptions;
    const vector<unique_ptr<WellRectangle<double>  > > & wellRects;
	vector<unique_ptr<WellDecoder> > wellDecoders;
    vector<WellDecoder *> decodedWells;
	unsigned decodedWellCount;
};

#endif /* DECODER_H_ */
