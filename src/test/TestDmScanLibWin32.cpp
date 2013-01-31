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
#include "decoder/WellDecoder.h"
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

TEST(TestDmScanLibWin32, isTwainAvailable) {
	DmScanLib dmScanLib(1);
	ASSERT_EQ(SC_SUCCESS, dmScanLib.isTwainAvailable());
}

TEST(TestDmScanLibWin32, selectSourceAsDefault) {
	DmScanLib dmScanLib(1);
	ASSERT_EQ(SC_SUCCESS, dmScanLib.selectSourceAsDefault());
}

TEST(TestDmScanLibWin32, getScannerCapability) {
	DmScanLib dmScanLib(1);
	int result = dmScanLib.getScannerCapability();

	ASSERT_EQ(CAP_IS_WIA, result & CAP_IS_WIA);
	ASSERT_EQ(CAP_DPI_300, result & CAP_DPI_300);
	ASSERT_EQ(CAP_DPI_400, result & CAP_DPI_400);
	ASSERT_EQ(CAP_DPI_600, result & CAP_DPI_600);
}

TEST(TestDmScanLibWin32, scanImage) {
	FLAGS_v = 3;

	const Point<double> originPt(0, 0);
	const Point<double> pt1(0.400, 0.265);
	std::unique_ptr<const Point<double> > pt1Neg = pt1.scale(-1);
	const Point<double> pt2(4.566, 3.020); 
	// convert to WIA coordinates
	std::unique_ptr<const Point<double> > pt2Wia = pt2.translate(*pt1Neg);

	std::string fname("tmpscan.bmp");
	std::wstring fnamew(fname.begin(), fname.end());
	DeleteFile(fnamew.c_str());

	DmScanLib dmScanLib(1);
	BoundingBox<double> scanRegion(pt1, *pt2Wia);
	const unsigned dpi = 300;
	int result = dmScanLib.scanImage(dpi, 0, 0, scanRegion, fname.c_str());

	EXPECT_EQ(SC_SUCCESS, result);
	Dib dib;
	dib.readFromFile(fname);
	EXPECT_EQ(dpi, dib.getDpi());

	BoundingBox<double> expectedImageSize(pt1, pt2);

	EXPECT_EQ(static_cast<unsigned>(expectedImageSize.getWidth() * dpi), 
		dib.getWidth());
	EXPECT_EQ(static_cast<unsigned>(expectedImageSize.getHeight() * dpi), 
		dib.getHeight());
}

TEST(TestDmScanLibWin32, scanImageInvalidDpi) {
	FLAGS_v = 3;

	const Point<double> originPt(0, 0);
	const Point<double> pt1(0.400, 0.265);

	DmScanLib dmScanLib(1);
	BoundingBox<double> scanRegion(originPt, pt1);

	int result = dmScanLib.scanImage(100, 0, 0, scanRegion, "tmpscan.bmp");
	EXPECT_EQ(SC_INVALID_DPI, result);

	result = dmScanLib.scanImage(200, 0, 0, scanRegion, "tmpscan.bmp");
	EXPECT_EQ(SC_INVALID_DPI, result);
}
TEST(TestDmScanLibWin32, scanFlatbed) {
	FLAGS_v = 3;

	std::string fname("flatbed.bmp");
	std::wstring fnamew(fname.begin(), fname.end());
	DeleteFile(fnamew.c_str());

	const unsigned dpi = 300;
	DmScanLib dmScanLib(1);
	int result = dmScanLib.scanFlatbed(dpi, 0, 0, fname.c_str());

	EXPECT_EQ(SC_SUCCESS, result);
	Dib dib;
	dib.readFromFile(fname);
	EXPECT_EQ(dpi, dib.getDpi());
}

TEST(TestDmScanLibWin32, scanAndDecodeInvalidDpi) {
	FLAGS_v = 3;

	const Point<double> pt1(0.396, 0.240);
	const Point<double> pt2(4.566, 3.089);
	const ScanRegion<double> scanRegion(pt1, pt2);

	std::unique_ptr<const BoundingBox<double> > wellsBbox(
		test::getWellsBoundingBox(*scanRegion.toBoundingBox()));
	std::unique_ptr<const ScanRegion<double> > scanRegionWia(
		test::getWiaBoundingBox(scanRegion));
	std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();

	std::vector<std::unique_ptr<WellRectangle<double> > > wellRects;
    test::getWellRectsForBoundingBox(200, 8, 12, *wellsBbox, wellRects);

	DmScanLib dmScanLib(1);
	int result = dmScanLib.scanAndDecode(200, 0, 0, *scanRegionWia, *decodeOptions, wellRects);

	EXPECT_EQ(SC_INVALID_DPI, result);
}

TEST(TestDmScanLibWin32, scanAndDecodeValidDpi) {
	FLAGS_v = 3;

	const Point<double> pt1(0.400, 0.265);
	const Point<double> pt2(4.566, 3.020); 
	const ScanRegion<double> scanRegion(pt1, pt2);
	std::unique_ptr<const BoundingBox<double> > wellsBbox(
		test::getWellsBoundingBox(*scanRegion.toBoundingBox()));
	std::unique_ptr<const ScanRegion<double> > scanRegionWia(
		test::getWiaBoundingBox(scanRegion));
	std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();

	std::vector<std::unique_ptr<WellRectangle<double> > > wellRects;

    test::getWellRectsForBoundingBox(600, 8, 12, *wellsBbox, wellRects);
	DmScanLib dmScanLib(1);
	int result = dmScanLib.scanAndDecode(600, 0, 0, *scanRegionWia, *decodeOptions, wellRects);

	EXPECT_EQ(SC_SUCCESS, result);
	EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

	const std::map<std::string, const WellDecoder *> & decodedWells =
		dmScanLib.getDecodedWells();

   	for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells.begin();
  			ii != decodedWells.end(); ++ii) {
      	const dmscanlib::WellDecoder & wellDecoder = *(ii->second);
		VLOG(1) << wellDecoder.getLabel() << ": " << wellDecoder.getMessage();
	}

	if (dmScanLib.getDecodedWellCount() > 0) {
		VLOG(1) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
	}
}

TEST(TestDmScanLibWin32, scanAndDecodeMultiple) {
	FLAGS_v = 1;

	const Point<double> pt1(0.400, 0.265);
	const Point<double> pt2(4.566, 3.020); 
	const ScanRegion<double> scanRegion(pt1, pt2);
	std::unique_ptr<const BoundingBox<double> > wellsBbox(
		test::getWellsBoundingBox(*scanRegion.toBoundingBox()));
	std::unique_ptr<const ScanRegion<double> > scanRegionWia(
		test::getWiaBoundingBox(scanRegion));
	std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();

	std::vector<std::unique_ptr<WellRectangle<double> > > wellRects;

    test::getWellRectsForBoundingBox(600, 8, 12, *wellsBbox, wellRects);
	DmScanLib dmScanLib(1);
	int result = dmScanLib.scanAndDecode(600, 0, 0, *scanRegionWia, *decodeOptions, wellRects);

	EXPECT_EQ(SC_SUCCESS, result);
	EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

	std::map<const std::string, std::string> lastResults;
	const std::map<std::string, const WellDecoder *> & decodedWells =
		dmScanLib.getDecodedWells();

   	for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells.begin();
  			ii != decodedWells.end(); ++ii) {
      	const dmscanlib::WellDecoder & wellDecoder = *(ii->second);
		lastResults[wellDecoder.getLabel()] = wellDecoder.getMessage();
	}

	result = dmScanLib.scanAndDecode(600, 0, 0, *scanRegionWia, *decodeOptions, wellRects);
	const std::map<std::string, const WellDecoder *> & decodedWells2 =
		dmScanLib.getDecodedWells();
   	for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells2.begin();
  			ii != decodedWells2.end(); ++ii) {
      	const dmscanlib::WellDecoder & wellDecoder = *(ii->second);

		if (lastResults.find(wellDecoder.getLabel()) != lastResults.end()) {
			EXPECT_EQ(lastResults[wellDecoder.getLabel()], wellDecoder.getMessage());
		}
	}
}

} /* namespace */
