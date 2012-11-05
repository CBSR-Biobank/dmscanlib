/*
 * TestDmScanLib.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#include "test/TestCommon.h"
#include "DmScanLib.h"
#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellRectangle.h"
#include "dib/Dib.h"

#include <stdexcept>
#include <stddef.h>
#include <sys/types.h>

#ifdef _VISUALC_
#   pragma warning(disable : 4996)
#else
#   include <dirent.h>
#endif

#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace dmscanlib;

namespace {

TEST(TestDmScanLibWin32, scanAndDecodeInvalidImage) {
	FLAGS_v = 3;

	Point<double> pt1(0.396, 0.240);
	Point<double> pt2(4.566, 3.089);
	BoundingBox<double> bbox(pt1, pt2);
	std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();

	std::vector<std::unique_ptr<WellRectangle<double> > > wellRects;
    test::getWellRectsForBoundingBox(200, 8, 12, bbox, wellRects);

	DmScanLib dmScanLib(1);
	int result = dmScanLib.scanAndDecode(200, 0, 0, bbox, *decodeOptions, wellRects);

	EXPECT_EQ(SC_INVALID_DPI, result);
}

} /* namespace */
