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

#include "BarcodeInfo.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "BinRegion.h"
#include "cxtypes.h"

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif


BarcodeInfo::BarcodeInfo() {
	postRect.x = -1;
	postRect.y = -1;
	postRect.width = 0;
	postRect.height = 0;
}

BarcodeInfo::~BarcodeInfo() {

}

void BarcodeInfo::postProcess(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) {
	UA_ASSERT_NOT_NULL(dec);
	UA_ASSERT_NOT_NULL(reg);
	UA_ASSERT_NOT_NULL(msg);

	str.assign((char *)msg->output, msg->outputIdx);

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
	getPostProcessBoundingBox();
}

void BarcodeInfo::setPreProcessBoundingBox(CvRect & rect) {
	preRect = rect;
}

CvRect & BarcodeInfo::getPreProcessBoundingBox() {
	return preRect;
}

CvRect & BarcodeInfo::getPostProcessBoundingBox() {
	DmtxPixelLoc topLeft;
	DmtxPixelLoc botRight;

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

	postRect.x = topLeft.X;
	postRect.y = topLeft.Y;
	postRect.width = botRight.X - topLeft.X;
	postRect.height = botRight.Y - topLeft.Y;

	return postRect;
}

void BarcodeInfo::translate(int x, int y){
	p00.X += x;
	p00.Y += y;

	p10.X += x;
	p10.Y += y;

	p11.X += x;
	p11.Y += y;

	p01.X += x;
	p01.Y += y;

	getPostProcessBoundingBox();
}

ostream & operator<<(ostream &os, BarcodeInfo & m) {
	os << "\"" << m.str	<< "\" (" << m.p00.X << ", " << m.p00.Y << "), "
	<< "(" << m.p10.X << ", " << m.p10.Y << "), "
	<< "(" << m.p11.X << ", " << m.p11.Y << "), "
	<< "(" << m.p01.X << ", " << m.p01.Y << ")";
	return os;
}
