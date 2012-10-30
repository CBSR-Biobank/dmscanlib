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
#   define _CRTDBG_MAP_ALLOC
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
#include <gtest/gtest.h>
#include <stdexcept>

#ifdef USE_NVWA
#   include <limits>
#   include "debug_new.h"
#endif

#ifdef WIN32
#   define DIR_SEP_CHR '\\'
#else
#   define DIR_SEP_CHR '/'
#endif

using namespace dmscanlib;

namespace {
class TestApp : public ::testing::Test {
protected:
    TestApp() {
    	DmScanLib::configLogging(3, false);
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
    Point<unsigned> horTranslation(wellWidth, 0);
    Point<unsigned> verTranslation(0, wellHeight);
    BoundingBox<unsigned> bbox(0, 0, wellWidth, wellHeight);

    for (int row = 0; row < 8; ++row) {
    	std::unique_ptr<const Point<unsigned>> scaledVertTranslation = verTranslation.scale(row);
        std::unique_ptr<const BoundingBox<unsigned> > bboxTranslated =
        		bbox.translate(*scaledVertTranslation);

        for (int col = 0; col < 12; ++col) {
        	std::ostringstream label;
        	label << (char) ('A' + row) << 12 - col;

            std::unique_ptr<WellRectangle<unsigned> > wellRect(
            		new WellRectangle<unsigned>(label.str().c_str(), *bboxTranslated));
            VLOG(3) << *wellRect;
            wellRects.push_back(std::move(wellRect));
            bboxTranslated = bboxTranslated->translate(horTranslation);
        }
    }
}

/*
 * Test for invalid rect
 */
TEST_F(TestApp, ScanImageInvalidRect) {
    ASSERT_THROW(new WellRectangle<double>("A12", 0, 0, 0, 0),
	std::invalid_argument);
}

TEST_F(TestApp, ScanImage) {

    std::string fname(getenv("HOME"));
    fname.append("/Dropbox/CBSR/scanlib/testImages/ohs_pallet.bmp");

    std::vector<std::unique_ptr<WellRectangle<unsigned> > > wellRects;

    getWellRectsForSbsPalletImage(fname, wellRects);

//	wellRects.push_back(std::unique_ptr<WellRectangle<unsigned> >(
//			new WellRectangle<unsigned>("A12", 10, 24, 130, 120)));
//
//	wellRects.push_back(std::unique_ptr<WellRectangle<unsigned> >(
//			new WellRectangle<unsigned>("A11", 150, 24, 250, 120)));

    DecodeOptions decodeOptions(0.085, 10, 5, 10, 1, 0.345);

    DmScanLib dmScanLib(1);
    int result = dmScanLib.decodeImageWells(fname.c_str(), decodeOptions, wellRects);

    EXPECT_EQ(SC_SUCCESS, result);
    EXPECT_TRUE(dmScanLib.getDecodedWells().size() > 0);

    VLOG(1) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
}

}  // namespace

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
