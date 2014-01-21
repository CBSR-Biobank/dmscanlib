/*
 * TestDmScanLib.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "test/TestCommon.h"
#include "DmScanLib.h"
#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellRectangle.h"
#include "decoder/WellDecoder.h"
#include "Image.h"

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

	const cv::Point_<float> pt1(0.400, 0.265);
	const cv::Point_<float> pt2(4.566, 3.020); 
	const ScanRegion<float> scanRegion(pt1, pt2);
	std::unique_ptr<const BoundingBox<float> > wellsBbox(
		test::getWellsBoundingBox(*scanRegion.toBoundingBox()));
	std::unique_ptr<const ScanRegion<float> > scanBbox(
		test::getWiaBoundingBox(scanRegion));
	std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();

	std::string fname("tmpscan.png");
	std::wstring fnamew(fname.begin(), fname.end());
	DeleteFile(fnamew.c_str());

	DmScanLib dmScanLib(1);
	const unsigned dpi = 300;
	int result = dmScanLib.scanImage(dpi, 0, 0, *scanBbox, fname.c_str());

	EXPECT_EQ(SC_SUCCESS, result);
	Image dib(fname.c_str());

	BoundingBox<float> expectedImageSize(pt1, pt2);

	cv::Size size = dib.size();

	EXPECT_EQ(static_cast<unsigned>(expectedImageSize.getWidth() * dpi), size.width);
	EXPECT_EQ(static_cast<unsigned>(expectedImageSize.getHeight() * dpi), size.height);
}

TEST(TestDmScanLibWin32, scanImageBadParams) {
	FLAGS_v = 3;	

	const cv::Point_<float> pt1(0.400, 0.265);
	const cv::Point_<float> pt2(4.566, 3.020); 
	const ScanRegion<float> scanRegion(pt1, pt2);
	std::unique_ptr<const BoundingBox<float> > wellsBbox(
		test::getWellsBoundingBox(*scanRegion.toBoundingBox()));
	std::unique_ptr<const ScanRegion<float> > scanBbox(
		test::getWiaBoundingBox(scanRegion));
	std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();

	std::string fname("tmpscan.png");
	std::wstring fnamew(fname.begin(), fname.end());
	DeleteFile(fnamew.c_str());

	DmScanLib dmScanLib(1);
    ASSERT_THROW(dmScanLib.scanImage(300, 0, 0, *scanBbox, NULL), std::invalid_argument);

	int result = dmScanLib.scanImage(0, 0, 0, *scanBbox, fname.c_str());
	EXPECT_EQ(SC_INVALID_DPI, result);
}

TEST(TestDmScanLibWin32, scanImageInvalidDpi) {
	FLAGS_v = 3;

	const cv::Point_<float> originPt(0, 0);
	const cv::Point_<float> pt1(0.400, 0.265);

	DmScanLib dmScanLib(1);
	ScanRegion<float> scanRegion(originPt, pt1);

	int result = dmScanLib.scanImage(100, 0, 0, scanRegion, "tmpscan.png");
	EXPECT_EQ(SC_INVALID_DPI, result);

	result = dmScanLib.scanImage(200, 0, 0, scanRegion, "tmpscan.png");
	EXPECT_EQ(SC_INVALID_DPI, result);
}

TEST(TestDmScanLibWin32, scanFlatbed) {
	FLAGS_v = 3;

	std::string fname("flatbed.png");
	std::wstring fnamew(fname.begin(), fname.end());
	DeleteFile(fnamew.c_str());

	const unsigned dpi = 300;
	DmScanLib dmScanLib(1);
	int result = dmScanLib.scanFlatbed(dpi, 0, 0, fname.c_str());

	EXPECT_EQ(SC_SUCCESS, result);
	Image dib(fname.c_str());
	EXPECT_TRUE(dib.isValid());
}

TEST(TestDmScanLibWin32, scanFlatbedBadParams) {
	FLAGS_v = 3;

	std::string fname("flatbed.png");
	std::wstring fnamew(fname.begin(), fname.end());
	DeleteFile(fnamew.c_str());

	DmScanLib dmScanLib(1);
    ASSERT_THROW(dmScanLib.scanFlatbed(300, 0, 0, NULL), std::invalid_argument);

	int result = dmScanLib.scanFlatbed(0, 0, 0, fname.c_str());
	EXPECT_EQ(SC_INVALID_DPI, result);
}

TEST(TestDmScanLibWin32, scanAndDecodeValidDpi) {
	FLAGS_v = 3;

	const unsigned dpi = 600;
	const cv::Point_<float> pt1(0.400, 0.265);
	const cv::Point_<float> pt2(4.566, 3.020); 
	const ScanRegion<float> scanRegion(pt1, pt2);
	std::unique_ptr<const ScanRegion<float> > scanRegionWia(
		test::getWiaBoundingBox(scanRegion));
	std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();

	const cv::Point_<unsigned> wrPt1(0, 0);
	const cv::Point_<unsigned> wrPt2(
		static_cast<unsigned>(dpi * (pt2.x - pt1.x)), 
		static_cast<unsigned>(dpi * (pt2.y - pt1.y)));

	std::unique_ptr<const BoundingBox<unsigned> > wellsBbox(
		new BoundingBox<unsigned>(wrPt1, wrPt2));

	std::vector<std::unique_ptr<const WellRectangle<float> > > wellRects;

    test::getWellRectsForBoundingBox(*wellsBbox, 8, 12, wellRects);
	DmScanLib dmScanLib(1);
	int result = dmScanLib.scanAndDecode(dpi, 0, 0, *scanRegionWia, *decodeOptions, wellRects);

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

	const unsigned dpi = 600;
	const cv::Point_<float> pt1(0.400, 0.265);
	const cv::Point_<float> pt2(4.566, 3.020); 

	const ScanRegion<float> scanRegion(pt1, pt2);
	std::unique_ptr<const ScanRegion<float> > scanRegionWia(
		test::getWiaBoundingBox(scanRegion));
	std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();

	const cv::Point_<unsigned> wrPt1(0, 0);
	const cv::Point_<unsigned> wrPt2(
		static_cast<unsigned>(dpi * (pt2.x - pt1.x)), 
		static_cast<unsigned>(dpi * (pt2.y - pt1.y)));

	std::unique_ptr<const BoundingBox<unsigned> > wellsBbox(
		new BoundingBox<unsigned>(wrPt1, wrPt2));

	std::vector<std::unique_ptr<const WellRectangle<float> > > wellRects;

    test::getWellRectsForBoundingBox(*wellsBbox, 8, 12, wellRects);
	DmScanLib dmScanLib(1);
	int result = dmScanLib.scanAndDecode(dpi, 0, 0, *scanRegionWia, *decodeOptions, wellRects);

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
