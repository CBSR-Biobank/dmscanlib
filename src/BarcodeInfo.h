#ifndef MESSABE_INFO_H_
#define MESSABE_INFO_H_

/*
 * Calibrator.h
 *
 *  Created on: 5-Jun-2009
 *      Author: loyola
 */

#include "Decoder.h"
#include "dmtx.h"
#include "UaLogger.h"
#include "UaAssert.h"

#include <string>
#include <limits>

using namespace std;

class Dib;
class BinRegion;

class BarcodeInfo {
public:
	BarcodeInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg);
	~BarcodeInfo();

	string & getMsg() {
		return str;
	}

	void getCorners(DmtxVector2 & p00, DmtxVector2 & p10,
			DmtxVector2 & p11, DmtxVector2 & p01);
	DmtxPixelLoc & getTopLeftCorner();
	DmtxPixelLoc & getBotRightCorner();

	void setColBinRegion(BinRegion * c) {
		UA_ASSERT_NOT_NULL(c);
		colBinRegion = c;
	}

	BinRegion & getColBinRegion() {
		UA_ASSERT_NOT_NULL(colBinRegion);
		return *colBinRegion;
	}

	void setRowBinRegion(BinRegion * c) {
		UA_ASSERT_NOT_NULL(c);
		rowBinRegion = c;
	}

	BinRegion & getRowBinRegion() {
		UA_ASSERT_NOT_NULL(rowBinRegion);
		return *rowBinRegion;
	}

	static void removeItems(vector<BarcodeInfo *>  & msgInfos);
	static void debugShowItems(vector<BarcodeInfo *>  & msgInfos);

private:
	string str;
	DmtxVector2 p00, p10, p11, p01;
	BinRegion * colBinRegion;
	BinRegion * rowBinRegion;
	DmtxPixelLoc topLeft;
	DmtxPixelLoc botRight;

	void getBoundingBox() ;

	friend ostream & operator<<(ostream & os, BarcodeInfo & m);
	friend struct BarcodeInfoSort;
};

ostream & operator<<(ostream &os, BarcodeInfo & m);

struct BarcodeInfoSort {
	bool operator()(BarcodeInfo* const& a, BarcodeInfo* const& b);
};

#endif /* MESSABE_INFO_H_ */

