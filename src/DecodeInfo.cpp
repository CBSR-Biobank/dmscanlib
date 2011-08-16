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

#include "DecodeInfo.h"
#include "UaLogger.h"
#include "UaAssert.h"
#include "cxtypes.h"

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

DecodeInfo::DecodeInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) {
    UA_ASSERT_NOT_NULL(dec);
    UA_ASSERT_NOT_NULL(reg);
    UA_ASSERT_NOT_NULL(msg);

    DmtxVector2 p00, p10, p11, p01;

    str.assign((char *) msg->output, msg->outputIdx);

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

    DmtxVector2 * p[4] = { &p00, &p10, &p11, &p01 };

    for (unsigned i = 0; i < 4; ++i) {
        corners[i].x = static_cast<int>(p[i]->X);
        corners[i].y = static_cast<int>(p[i]->Y);
    }
}

DecodeInfo::~DecodeInfo() {
}

bool DecodeInfo::isValid() {
    return (str.length() > 0);
}

const string & DecodeInfo::getMsg() const {
    return str;
}

void DecodeInfo::getCorners(std::vector<const CvPoint *> & crnr) const {
    crnr.resize(4);
    for (unsigned i = 0; i < 4; ++i) {
        crnr[i] = &corners[i];
    }
}

bool DecodeInfo::equals(DecodeInfo * other) {
    return strcmp(this->getMsg().c_str(), other->getMsg().c_str()) == 0;
}

ostream & operator<<(ostream &os, DecodeInfo & m) {
    os << "\"" << m.str << "\" (" << m.corners[0].x << ", " << m.corners[0].y
       << "), " << "(" << m.corners[2].x << ", " << m.corners[2].y << "), "
       << "(" << m.corners[3].x << ", " << m.corners[3].y << "), " << "("
       << m.corners[1].x << ", " << m.corners[1].y << ")";
    return os;
}
