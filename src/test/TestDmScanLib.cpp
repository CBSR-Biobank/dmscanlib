/*
 * TestDmScanLib.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#include "DmScanLib.h"
#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "test/TestCommon.h"

#include <stdexcept>
#include <stddef.h>
#include <sys/types.h>

#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace dmscanlib;

namespace {

TEST(TestDmScanLib, invalidRects) {
	FLAGS_v = 0;

    std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();
    std::vector<std::unique_ptr<WellRectangle<double> > > wellRects;
	DmScanLib dmScanLib(1);
	int result = dmScanLib.decodeImageWells("testImages/96tubes.bmp", *decodeOptions, wellRects);
	EXPECT_EQ(SC_INVALID_NOTHING_TO_DECODE, result);
}

TEST(TestDmScanLib, invalidImage) {
	FLAGS_v = 0;

	Point<double> pt1(0,0);
	Point<double> pt2(10,10);
	BoundingBox<double> bbox(pt1, pt2);
	std::unique_ptr<WellRectangle<double> > wrect(new WellRectangle<double>("label", bbox));

	std::vector<std::unique_ptr<WellRectangle<double> > > wellRects;
    wellRects.push_back(std::move(wrect));

    std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();
	DmScanLib dmScanLib(1);
	int result = dmScanLib.decodeImageWells("xyz.bmp", *decodeOptions, wellRects);
	EXPECT_EQ(SC_INVALID_IMAGE, result);
}

TEST(TestDmScanLib, decodeImage) {
	FLAGS_v = 3;

	std::string fname("testImages/edge_tubes.bmp");

	DmScanLib dmScanLib(1);
	int result = test::decodeImage(fname, dmScanLib);

	EXPECT_EQ(SC_SUCCESS, result);
	EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

	if (dmScanLib.getDecodedWellCount() > 0) {
		VLOG(1) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
	}
}

TEST(TestDmScanLib, decodeAllImages) {
	FLAGS_v = 1;

    std::string dirname("testImages");
    std::vector<std::string> filenames;
	bool result = test::getTestImageFileNames(dirname, filenames);
	EXPECT_EQ(true, result);

	int decodeResult;

	for (unsigned i = 0, n = filenames.size(); i < n; ++i) {
		VLOG(1) << "test image: " << filenames[i];

		util::DmTime start;
		DmScanLib dmScanLib(1);
		decodeResult = test::decodeImage(filenames[i], dmScanLib);
		util::DmTime end;

		std::unique_ptr<util::DmTime> difftime = end.difftime(start);

		EXPECT_EQ(SC_SUCCESS, decodeResult);
		EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

		VLOG(1) << "test image: " << filenames[i] << ", wells decoded: "
				<< dmScanLib.getDecodedWellCount()
				<< " time taken: " << *difftime << " sec";
	}
}

} /* namespace */
