/*
 * Calibrator.cpp
 *
 *  Created on: 5-Jun-2009
 *      Author: loyola
 */

#include "BarcodeInfo.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "BinRegion.h"

#if defined(USE_MPATROL)
#   include "mpatrol.h"
#endif


BarcodeInfo::BarcodeInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) :
	colBinRegion(NULL), rowBinRegion(NULL) {
	str.append((char *) msg->output, msg->outputIdx);

	int height = dmtxDecodeGetProp(dec, DmtxPropHeight);
	p00.X = p00.Y = p10.Y = p01.X = 0.0;
	p10.X = p01.Y = p11.X = p11.Y = 1.0;
	dmtxMatrix3VMultiplyBy(&p00, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p10, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p11, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p01, reg->fit2raw);

	p00.Y = height - 1 - p00.Y;
	p10.Y = height - 1 - p10.Y;
	p11.Y = height - 1 - p11.Y;
	p01.Y = height - 1 - p01.Y;
	getBoundingBox();
}

BarcodeInfo::~BarcodeInfo() {
}


void BarcodeInfo::getCorners(DmtxVector2 & rp00, DmtxVector2 & rp10,
		DmtxVector2 & rp11, DmtxVector2 & rp01) {
	rp00 = p00;
	rp10 = p10;
	rp11 = p11;
	rp01 = p01;
}

void BarcodeInfo::getBoundingBox() {
	topLeft.X = (int) p00.X;
	topLeft.Y = (int) p00.Y;
	botRight.X = (int) p00.X;
	botRight.Y = (int) p00.Y;

	if ((int) p10.X < topLeft.X) {
		topLeft.X = (int) p10.X;
	}
	if ((int) p11.X < topLeft.X) {
		topLeft.X = (int) p11.X;
	}
	if ((int) p01.X < topLeft.X) {
		topLeft.X = (int) p01.X;
	}

	if ((int) p10.Y < topLeft.Y) {
		topLeft.Y = (int) p10.Y;
	}
	if ((int) p11.Y < topLeft.Y) {
		topLeft.Y = (int) p11.Y;
	}
	if ((int) p01.Y < topLeft.Y) {
		topLeft.Y = (int) p01.Y;
	}

	if ((int) p10.X > botRight.X) {
		botRight.X = (int) p10.X;
	}
	if ((int) p11.X > botRight.X) {
		botRight.X = (int) p11.X;
	}
	if ((int) p01.X > botRight.X) {
		botRight.X = (int) p01.X;
	}

	if ((int) p10.Y > botRight.Y) {
		botRight.Y = (int) p10.Y;
	}
	if ((int) p11.Y > botRight.Y) {
		botRight.Y = (int) p11.Y;
	}
	if ((int) p01.Y > botRight.Y) {
		botRight.Y = (int) p01.Y;
	}
}

DmtxPixelLoc & BarcodeInfo::getTopLeftCorner() {
	return topLeft;
}

DmtxPixelLoc & BarcodeInfo::getBotRightCorner() {
	return botRight;
}



void BarcodeInfo::removeItems(vector<BarcodeInfo *>  & msgInfos) {
	while (msgInfos.size() > 0) {
		BarcodeInfo * info = msgInfos.back();
		msgInfos.pop_back();
		delete info;
	}
}

void BarcodeInfo::debugShowItems(vector<BarcodeInfo *>  & msgInfos) {
	unsigned numTags = msgInfos.size();
	UA_DOUT(1, 1, "debugTags: tags found: " << numTags);
	for (unsigned i = 0; i < numTags; ++i) {
		BarcodeInfo & info = *msgInfos[i];
		UA_DOUT(1, 1, "debugTags: tag " << i << ": " << info);
	}
}

ostream & operator<<(ostream &os, BarcodeInfo & m) {
	os << "\"" << m.str	<< "\" (" << m.p00.X << ", " << m.p00.Y << "), "
	<< "(" << m.p10.X << ", " << m.p10.Y << "), "
	<< "(" << m.p11.X << ", " << m.p11.Y << "), "
	<< "(" << m.p01.X << ", " << m.p01.Y << ")";
	return os;
}

/* should only be invoked by Decoder::sortRegions().
 *
 * Need to sort right to left, then top to bottom. That is how Biobank
 * numbers the tubes.
 */
bool BarcodeInfoSort::operator()(BarcodeInfo* const& a, BarcodeInfo* const& b) {
	UA_ASSERT_NOT_NULL(a->colBinRegion);
	UA_ASSERT_NOT_NULL(a->rowBinRegion);
	UA_ASSERT_NOT_NULL(b->colBinRegion);
	UA_ASSERT_NOT_NULL(b->rowBinRegion);

	unsigned aColRank = a->getColBinRegion().getRank();
	unsigned aRowRank = a->getRowBinRegion().getRank();
	unsigned bColRank = b->getColBinRegion().getRank();
	unsigned bRowRank = b->getRowBinRegion().getRank();

#ifndef _VISUALC_
	UA_ASSERT(aColRank != numeric_limits<unsigned int>::max());
	UA_ASSERT(aRowRank != numeric_limits<unsigned int>::max());
	UA_ASSERT(bColRank != numeric_limits<unsigned int>::max());
	UA_ASSERT(bRowRank != numeric_limits<unsigned int>::max());
#endif

	int diff = aRowRank - bRowRank;
	if (diff == 0) {
		return (aColRank > bColRank);
	}
	return (diff < 0);
}
