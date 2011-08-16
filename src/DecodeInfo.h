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
#include <vector>

#ifdef _VISUALC_
#   include <memory>
#else
#   include <tr1/memory>
#endif

using namespace std;

class Dib;
class BinRegion;
struct CvRect;

class DecodeInfo {
public:
	DecodeInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	~DecodeInfo();

	bool isValid();

	const string & getMsg() const;

    void getCorners(std::vector<const CvPoint *> & corners) const;

	bool equals(DecodeInfo * other);


private:
	string str;
	CvPoint corners[4];

	friend ostream & operator<<(ostream & os, DecodeInfo & m);
};

#endif /* BARCODE_INFO_H_ */

