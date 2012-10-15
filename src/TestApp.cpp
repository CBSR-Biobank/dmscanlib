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
#include "utils/SimpleOpt.h"

#include <iostream>
#include <vector>
#include <gtest/gtest.h>

#ifdef USE_NVWA
#   include <limits>
#   include "debug_new.h"
#endif

#ifdef WIN32
#   define DIR_SEP_CHR '\\'
#else
#   define DIR_SEP_CHR '/'
#endif

using namespace std;

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

TEST_F(TestApp, ScanImage) {

    vector<unique_ptr<WellRectangle<double>  > > wellRects;

	wellRects.push_back(unique_ptr<WellRectangle<double> >(
			new WellRectangle<double>("A12", 10.0 / 400.0, 24.0 / 400.0,
	                130.0 / 400.0, 120.0 / 400.0)));

	wellRects.push_back(unique_ptr<WellRectangle<double> >(
			new WellRectangle<double>("A11", 150.0 / 400.0, 24.0 / 400.0,
            250.0 / 400.0, 120.0 / 400.0)));

    DecodeOptions decodeOptions(0.085, 15, 5, 10, 0.345);

    string fname(getenv("HOME"));
    fname.append("/Dropbox/CBSR/scanlib/testImages/96tubes_cropped.bmp");

    DmScanLib dmScanLib(3);
    dmScanLib.decodeImageWells(fname.c_str(), decodeOptions, wellRects);
}

}  // namespace

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
