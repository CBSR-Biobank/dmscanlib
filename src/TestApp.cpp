/*******************************************************************************
 * Canadian Biosample Repository
 *
 * DmScanLib project
 *
 * Multi-platform application for scanning and decoding datamatrix 2D barcodes.
 *
 * ---------------------------------------------------------------------------
 * Dmscanlib is a software library and standalone application that scans
 * and decodes libdmtx compatible test-tubes. It is currently designed
 * to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 * Copyright (C) 2010 Canadian Biosample Repository
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#ifdef _VISUALC_
//#   define _CRTDBG_MAP_ALLOC
#   pragma warning(disable : 4996)
//Scan for memory leaks in visual studio
#   ifdef _DEBUG
#      define _CRTDBG_MAP_ALLOC
#      include <stdlib.h>
#      include <crtdbg.h>
#   endif
#endif

#include "DmScanLib.h"
#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellRectangle.h"
#include "dib/Dib.h"
#include "imgscanner/ImgScanner.h"

#include <iostream>
#include <vector>
#include <stdexcept>
#include <stddef.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#ifdef USE_NVWA
#   include <limits>
#   include "debug_new.h"
#endif

#ifdef WIN32
#   define DIR_SEP_CHR '\\'
#else
#   define DIR_SEP_CHR '/'
#    include <dirent.h>
#endif

using namespace dmscanlib;

namespace {

class TestApp : public ::testing::Test {
protected:
    TestApp() {
    }

    ~TestApp() {

    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

private:
};

/*
 * Gets file names for all the test images in the Dropbox folder. Only MS
 * Windows bitmap files are included.
 */
bool getTestImageFileNames(std::string dir, std::vector<std::string> & filenames) {
#ifndef _VISUALC_
	DIR * dp;
	dirent * dirp;

	dp = opendir(dir.c_str());
	if (dp == NULL) return false;

	VLOG(3) << "getting files from directory: " << dir;

	while ((dirp = readdir(dp)) != NULL) {
		if (((dirp->d_type == DT_DIR) && (dirp->d_name[0] != '.'))) {
			std::string subdirname;
			subdirname.append(dir).append("/").append(dirp->d_name);
			getTestImageFileNames(subdirname, filenames);
		} else if (dirp->d_type == DT_REG) {
			std::string basename(dirp->d_name);

			if (basename.find(".bmp") != std::string::npos) {
				filenames.push_back(std::string(dir).append("/").append(dirp->d_name));
			}
		}
	}
	closedir(dp);
#endif
	return true;
}

/*
 * Assumes image has 96 well plates in 8 rows by 12 columns
 */
void getWellRectsForSbsPalletImage(std::string & fname,
		std::vector<std::unique_ptr<WellRectangle<unsigned> > > & wellRects) {

	Dib image;
	bool readResult = image.readFromFile(fname);
	if (!readResult) {
		throw std::invalid_argument("could not load image");
	}

    double width = image.getWidth();
    double height = image.getHeight();
    double wellWidth = width / 12.0;
    double wellHeight = height / 8.0;
    Point<unsigned> horTranslation(static_cast<unsigned>(wellWidth), 0);
    Point<unsigned> verTranslation(0, static_cast<unsigned>(wellHeight));

    // round off the bounding box so image dimensions are not exceeded
    BoundingBox<unsigned> bbox(0, 0, static_cast<unsigned>(wellWidth - 0.5), 
		static_cast<unsigned>(wellHeight - 0.5));

    for (int row = 0; row < 8; ++row) {
    	std::unique_ptr<const Point<unsigned>> scaledVertTranslation = verTranslation.scale(row);
        std::unique_ptr<const BoundingBox<unsigned> > bboxTranslated =
        		bbox.translate(*scaledVertTranslation);

        for (int col = 0; col < 12; ++col) {
        	std::ostringstream label;
        	label << (char) ('A' + row) << 12 - col;

            std::unique_ptr<WellRectangle<unsigned> > wellRect(
            		new WellRectangle<unsigned>(label.str().c_str(), *bboxTranslated));
            VLOG(9) << *wellRect;
            wellRects.push_back(std::move(wellRect));
            bboxTranslated = bboxTranslated->translate(horTranslation);
        }
    }
}

/*
 * Test for invalid rect
 */
TEST_F(TestApp, DecodeImageInvalidRect) {
    ASSERT_THROW(new WellRectangle<double>("A12", 0, 0, 0, 0),
	std::invalid_argument);
}

int decodeImage(std::string fname, DmScanLib & dmScanLib) {
    std::vector<std::unique_ptr<WellRectangle<unsigned> > > wellRects;

    getWellRectsForSbsPalletImage(fname, wellRects);

    double scanGap = 5;
    long squareDev = 15;
    long edgeThresh = 5;
    long corrections = 10;
    long shrink = 1;

    DecodeOptions decodeOptions(scanGap, squareDev, edgeThresh, corrections,
    	    shrink);
    return dmScanLib.decodeImageWells(fname.c_str(), decodeOptions, wellRects);
}

TEST_F(TestApp, DecodeImage) {
	FLAGS_v = 3;

    std::string fname(getenv("HOME"));
    fname.append("/Dropbox/CBSR/scanlib/testImages/edge_tubes.bmp");

    DmScanLib dmScanLib(1);
    int result = decodeImage(fname, dmScanLib);

    EXPECT_EQ(SC_SUCCESS, result);
    EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

    if (dmScanLib.getDecodedWellCount() > 0) {
    	VLOG(1) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
    }
}

TEST_F(TestApp, DecodeAllImages) {
	FLAGS_v = 1;

    std::string dirname(getenv("HOME"));
    dirname.append("/Dropbox/CBSR/scanlib/testImages");
    std::vector<std::string> filenames;
	bool result = getTestImageFileNames(dirname, filenames);
    EXPECT_EQ(true, result);

    int decodeResult;

    for (unsigned i = 0, n = filenames.size(); i < n; ++i) {
    	VLOG(1) << "test image: " << filenames[i];

		util::Time start;
        DmScanLib dmScanLib(1);
    	decodeResult = decodeImage(filenames[i], dmScanLib);
		util::Time end;

		std::unique_ptr<util::Time> difftime = end.difftime(start);

        EXPECT_EQ(SC_SUCCESS, decodeResult);
        EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

        VLOG(1) << "test image: " << filenames[i] << ", wells decoded: "
        		<< dmScanLib.getDecodedWellCount()
        		<< " time taken: " << *difftime;
    }
}

}  // namespace

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	DmScanLib::configLogging(1, false);
	return RUN_ALL_TESTS();
}
