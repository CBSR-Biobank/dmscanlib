/*
 * Calibrator.cpp
 *
 *  Created on: 5-Jun-2009
 *      Author: loyola
 */

#include "MessageInfo.h"
#include "UaDebug.h"
#include "BinRegion.h"


MessageInfo::MessageInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) :
	colBinRegion(NULL), rowBinRegion(NULL), topLeft(NULL), botRight(NULL) {
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
}

MessageInfo::~MessageInfo() {
}


void MessageInfo::getCorners(DmtxVector2 & rp00, DmtxVector2 & rp10,
		DmtxVector2 & rp11, DmtxVector2 & rp01) {
	rp00 = p00;
	rp10 = p10;
	rp11 = p11;
	rp01 = p01;
}

void MessageInfo::getCorners() {
	double minDist = numeric_limits<double>::max(), maxDist = 0;;
	unsigned min = 4, max = 4;

	double dist[4] = {
			dmtxVector2Mag(&p00),
			dmtxVector2Mag(&p10),
			dmtxVector2Mag(&p11),
			dmtxVector2Mag(&p01),
	};

	for (unsigned i = 0; i < 4; ++i) {
		if (dist[i] < minDist) {
			minDist = dist[i];
			min = i;
		}
		if (dist[i] > maxDist) {
			maxDist = dist[i];
			max = i;
		}
	}

	switch (min) {
	case 0: topLeft = &p00; break;
	case 1: topLeft = &p10; break;
	case 2: topLeft = &p11; break;
	case 3: topLeft = &p01; break;
	default:
		UA_ASSERTS(false, "invalid value for min: " << min);
	}

	switch (max) {
	case 0: botRight = &p00; break;
	case 1: botRight = &p10; break;
	case 2: botRight = &p11; break;
	case 3: botRight = &p01; break;
	default:
		UA_ASSERTS(false, "invalid value for max: " << max);
	}
}

DmtxVector2 & MessageInfo::getTopLeftCorner() {
	if (topLeft == NULL) {
		getCorners();
	}
	UA_ASSERT_NOT_NULL(topLeft);
	return *topLeft;
}

DmtxVector2 & MessageInfo::getBotRightCorner() {
	if (botRight == NULL) {
		getCorners();
	}
	UA_ASSERT_NOT_NULL(topLeft);
	return *botRight;
}

ostream & operator<<(ostream &os, MessageInfo & m) {
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
bool MessageInfoSort::operator()(MessageInfo* const& a, MessageInfo* const& b) {
	UA_ASSERT_NOT_NULL(a->colBinRegion);
	UA_ASSERT_NOT_NULL(a->rowBinRegion);
	UA_ASSERT_NOT_NULL(b->colBinRegion);
	UA_ASSERT_NOT_NULL(b->rowBinRegion);

	unsigned aColRank = a->getColBinRegion().getRank();
	unsigned aRowRank = a->getRowBinRegion().getRank();
	unsigned bColRank = b->getColBinRegion().getRank();
	unsigned bRowRank = b->getRowBinRegion().getRank();

	UA_ASSERT(aColRank != numeric_limits<unsigned>::max());
	UA_ASSERT(aRowRank != numeric_limits<unsigned>::max());
	UA_ASSERT(bColRank != numeric_limits<unsigned>::max());
	UA_ASSERT(bRowRank != numeric_limits<unsigned>::max());

	int diff = aRowRank - bRowRank;
	if (diff == 0) {
		return (aColRank > bColRank);
	}
	return (diff < 0);
}
