#ifndef BARCODE_INFO_H_
#define BARCODE_INFO_H_
/*
 Dmscanlib is a software library and standalone application that scans
 and decodes libdmtx compatible test-tubes. It is currently designed
 to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 Copyright (C) 2010 Canadian Biosample Repository

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "Decoder.h"
#include "dmtx.h"
#include "UaLogger.h"
#include "UaAssert.h"

#include <string>

#ifdef _VISUALC_
#   include <memory>
#else
#   include <tr1/memory>
#endif

using namespace std;

class Dib;
class BinRegion;
struct CvRect;

class BarcodeInfo {
public:
	BarcodeInfo();
	~BarcodeInfo();

	void postProcess(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);

	bool isValid();

	string & getMsg();

	bool equals(BarcodeInfo * other);

	void setPreProcessBoundingBox(std::tr1::shared_ptr<const CvRect> rect);
	std::tr1::shared_ptr<const CvRect> getPreProcessBoundingBox();
	std::tr1::shared_ptr<const CvRect> getPostProcessBoundingBox();
	void translate(int x, int y);

	unsigned getRow() {
		return row;
	}

	void setRow(unsigned r) {
		row = r;
	}

	unsigned getCol() {
		return col;
	}

	void setCol(unsigned c) {
		col = c;
	}


private:
	string str;
	DmtxVector2 p00, p10, p11, p01;
	std::tr1::shared_ptr<const CvRect> preRect;
	std::tr1::shared_ptr<CvRect> postRect;
	unsigned row;
	unsigned col;

	friend ostream & operator<<(ostream & os, BarcodeInfo & m);
};

#endif /* BARCODE_INFO_H_ */

