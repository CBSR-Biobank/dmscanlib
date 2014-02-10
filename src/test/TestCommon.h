#ifndef __INC_TEST_COMMON
#define __INC_TEST_COMMON

/*
 * TestCommon.h
 *
 *  Created on: 2012-11-05
 *      Author: nelson
 */

#include "decoder/DecodeOptions.h"
#include "decoder/WellRectangle.h"

#include <string>
#include <vector>
#include <memory>

namespace dmscanlib {

class DmScanLib;

namespace test {

enum Orientation { LANDSCAPE, PORTRAIT, ORIENTATION_MAX };

enum BarcodePosition { TUBE_TOPS, TUBE_BOTTOMS, BARCODE_POSITION_MAX };

bool getTestImageInfoFilenames(std::string dir, std::vector<std::string> & filenames);

void getWellRectsForBoundingBox(
        const cv::Rect & bbox,
        const unsigned rows,
        const unsigned cols,
        Orientation orientation,
        BarcodePosition position,
        std::vector<std::unique_ptr<const WellRectangle> > & wellRects);

std::unique_ptr<DecodeOptions> getDefaultDecodeOptions();

int decodeImage(std::string fname, DmScanLib & dmScanLib, unsigned rows, unsigned cols);

} /* namespace */

} /* namespace */

#endif /* __INC_TEST_COMMON */
