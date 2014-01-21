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
        const BoundingBox<unsigned> & bbox,
        const unsigned rows,
        const unsigned cols,
        std::vector<std::unique_ptr<const WellRectangle<float> > > & wellRects);

std::unique_ptr<DecodeOptions> getDefaultDecodeOptions();

int decodeImage(std::string fname, DmScanLib & dmScanLib, unsigned rows, unsigned cols);

std::unique_ptr<const BoundingBox<float>> getWellsBoundingBox(
        const BoundingBox<float> & bbox);

std::unique_ptr<const ScanRegion<float>> getWiaBoundingBox(
        const ScanRegion<float> & bbox);

} /* namespace */

} /* namespace */

#endif /* __INC_TEST_COMMON */
