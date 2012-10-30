/*
 * DmtxDecodeHelper.cpp
 *
 *  Created on: 2012-10-30
 *      Author: loyola
 */

#include "DmtxDecodeHelper.h"

#include <glog/logging.h>

namespace dmscanlib {

namespace decoder {

DmtxDecodeHelper::DmtxDecodeHelper(DmtxImage * dmtxImage, int scale) :
		dec(dmtxDecodeCreate(dmtxImage, scale))
{
	CHECK_NOTNULL(dec);
}


DmtxDecodeHelper::~DmtxDecodeHelper() {
	dmtxDecodeDestroy(&dec);
}

unsigned DmtxDecodeHelper::setProperty(int prop, int value) {
	CHECK_NOTNULL(dec);
	return dmtxDecodeSetProp(dec, prop, value);
}

} /* namespace decoder */


} /* namespace dmscanlib */
