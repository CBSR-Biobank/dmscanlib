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

bool getTestImageInfoFilenames(std::string dir, std::vector<std::string> & filenames);

void getWellRectsForBoundingBox(
        const cv::Rect & bbox,
        const unsigned rows,
        const unsigned cols,
        std::vector<std::unique_ptr<const WellRectangle> > & wellRects);

std::unique_ptr<DecodeOptions> getDefaultDecodeOptions();

int decodeImage(std::string fname, DmScanLib & dmScanLib, unsigned rows, unsigned cols);

std::unique_ptr<const cv::Rect_<float>> getWiaBoundingBox(
        const cv::Rect_<float> & bbox);

} /* namespace */

} /* namespace */

#endif /* __INC_TEST_COMMON */
