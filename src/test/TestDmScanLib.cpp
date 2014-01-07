/*
 * TestDmScanLib.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#include "DmScanLib.h"
#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellDecoder.h"
#include "test/TestCommon.h"
#include "test/ImageInfo.h"

#include <stdexcept>
#include <stddef.h>
#include <sys/types.h>
#include <sstream>
#include <iostream>
#include <fstream>

#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace dmscanlib;

namespace {

TEST(TestDmScanLib, invalidRects) {
	FLAGS_v = 0;

    std::unique_ptr<DecodeOptions> decodeOptions = 
		test::getDefaultDecodeOptions();
    std::vector<std::unique_ptr<const WellRectangle<double> > > wellRects;
	DmScanLib dmScanLib(1);
	int result = dmScanLib.decodeImageWells("testImages/8x12/96tubes.bmp", *decodeOptions, wellRects);
	EXPECT_EQ(SC_INVALID_NOTHING_DECODED, result);
}

TEST(TestDmScanLib, invalidImage) {
	FLAGS_v = 0;

	Point<double> pt1(0,0);
	Point<double> pt2(10,10);
	BoundingBox<double> bbox(pt1, pt2);
	std::unique_ptr<const WellRectangle<double> > wrect(new WellRectangle<double>("label", bbox));

	std::vector<std::unique_ptr<const WellRectangle<double> > > wellRects;
    wellRects.push_back(std::move(wrect));

    std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();
	DmScanLib dmScanLib(1);
	int result = dmScanLib.decodeImageWells("xyz.bmp", *decodeOptions, wellRects);
	EXPECT_EQ(SC_INVALID_IMAGE, result);
}

TEST(TestDmScanLib, decodeImage) {
	FLAGS_v = 3;

	//std::string fname("testImages/8x12/hardscan.bmp");
	//std::string fname("testImages/8x12/plate.bmp");
	//std::string fname("/home/nelson/Desktop/ohs_crash_image001.bmp");
	//std::string fname("testImages/10x10/10x10.bmp");
	std::string fname("/home/loyola/Desktop/ohs_pallet_crash.bmp");

	DmScanLib dmScanLib(1);
	int result = test::decodeImage(fname, dmScanLib, 8, 12);

	EXPECT_EQ(SC_SUCCESS, result);
	EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

	if (dmScanLib.getDecodedWellCount() > 0) {
		VLOG(1) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
	}
}

void writeDecodeAllResults(std::vector<std::string>  & testResults) {
	std::ofstream ofile("all_images_results.csv");

	for (unsigned i = 0, n = testResults.size(); i < n; ++i) {
		ofile <<  testResults[i] << std::endl;
	}
	ofile.close();
}

// check that the decoded message matches the one in the "nfo" file
void checkDecodeInfo(DmScanLib & dmScanLib, dmscanlib::test::ImageInfo & imageInfo) {
	const std::map<std::string, const WellDecoder *> & decodedWells = dmScanLib.getDecodedWells();
	for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells.begin();
			ii != decodedWells.end(); ++ii) {
		const WellDecoder & decodedWell = *(ii->second);
		const std::string & label = decodedWell.getLabel();
		const std::string * nfoDecodedMsg = imageInfo.getBarcodeMsg(label);

		if ((decodedWell.getMessage().length() > 0) && (nfoDecodedMsg != NULL)) {
			EXPECT_EQ(*nfoDecodedMsg, decodedWell.getMessage()) << "label: " << label;
		}
	}
}

TEST(TestDmScanLib, decodeFromInfo) {
	FLAGS_v = 1;

	std::string fnameInfoFile("testImages/8x12/plate.nfo");
	dmscanlib::test::ImageInfo imageInfo(fnameInfoFile);

	EXPECT_TRUE(imageInfo.isValid());

    std::vector<std::unique_ptr<const WellRectangle<double> > > wellRects;
    test::getWellRectsForBoundingBox(
    		imageInfo.getBoundingBox(),
    		imageInfo.getPalletRows(),
    		imageInfo.getPalletCols(),
    		wellRects);

	util::DmTime start;
	DmScanLib dmScanLib(0);
    std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();
    int decodeResult = dmScanLib.decodeImageWells(
    		imageInfo.getImageFilename().c_str(), *decodeOptions, wellRects);
	util::DmTime end;

	std::unique_ptr<util::DmTime> difftime = end.difftime(start);

	EXPECT_EQ(SC_SUCCESS, decodeResult);
	EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);
	checkDecodeInfo(dmScanLib, imageInfo);
}


//TEST(TestDmScanLib, DISABLED_decodeAllImages) {
TEST(TestDmScanLib, decodeAllImages) {
	FLAGS_v = 1;

    std::string dirname("testImages");
    std::vector<std::string> filenames;
	bool result = test::getTestImageFileNames(dirname, filenames);
	EXPECT_EQ(true, result);

	int decodeResult;
    std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();

    std::vector<std::string> testResults;
    testResults.push_back("#filename,decoded,total,ratio,time (sec)");

	for (unsigned i = 0, n = filenames.size(); i < n; ++i) {
		VLOG(1) << "test image: " << filenames[i];

		std::string & filename = filenames[i];
		std::string infoFilename(filename);
		std::size_t pos = filename.find("bmp");
		infoFilename.replace(pos, pos+3, "nfo");

		std::cout << "filename: " << filename << std::endl;

		dmscanlib::test::ImageInfo imageInfo(infoFilename);
		EXPECT_TRUE(imageInfo.isValid());

	    std::vector<std::unique_ptr<const WellRectangle<double> > > wellRects;
	    test::getWellRectsForBoundingBox(
	    		imageInfo.getBoundingBox(),
	    		imageInfo.getPalletRows(),
	    		imageInfo.getPalletCols(),
	    		wellRects);

		util::DmTime start;
		DmScanLib dmScanLib(0);
	    decodeResult = dmScanLib.decodeImageWells(filename.c_str(), *decodeOptions, wellRects);
		util::DmTime end;

		std::unique_ptr<util::DmTime> difftime = end.difftime(start);

		EXPECT_EQ(SC_SUCCESS, decodeResult);
		EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

		// check that the decoded message matches the one in the "nfo" file
		const std::map<std::string, const WellDecoder *> & decodedWells = dmScanLib.getDecodedWells();
		for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells.begin();
				ii != decodedWells.end(); ++ii) {
			const WellDecoder & decodedWell = *(ii->second);
			const std::string & label = decodedWell.getLabel();
			const std::string * nfoDecodedMsg = imageInfo.getBarcodeMsg(label);

			if ((decodedWell.getMessage().length() > 0) && (nfoDecodedMsg != NULL)) {
				EXPECT_EQ(*nfoDecodedMsg, decodedWell.getMessage()) << "label: " << label;
			}
		}

	    std::stringstream ss;
		ss << filenames[i]
		   << "," << dmScanLib.getDecodedWellCount()
		   << "," << imageInfo.getDecodedWellCount()
		   << "," << dmScanLib.getDecodedWellCount()
		        / static_cast<double>(imageInfo.getDecodedWellCount())
		   << "," << *difftime;
		testResults.push_back(ss.str());
	}

	writeDecodeAllResults(testResults);
}

} /* namespace */
