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

bool getTestImageFileNames(std::string dir, std::vector<std::string> & filenames);

void getWellRectsForBoundingBox(const unsigned dpi, const unsigned rows,
	const unsigned cols, const BoundingBox<double> & bbox,
	std::vector<std::unique_ptr<WellRectangle<double> > > & wellRects);

void getWellRectsForSbsPalletImage(std::string & fname, const unsigned rows,
	const unsigned cols, std::vector<std::unique_ptr<WellRectangle<double> > > & wellRects);

std::unique_ptr<DecodeOptions> getDefaultDecodeOptions();

int decodeImage(std::string fname, DmScanLib & dmScanLib);

std::unique_ptr<const BoundingBox<double>> getWellsBoundingBox(
	const BoundingBox<double> & bbox);

std::unique_ptr<const BoundingBox<double>> getWiaBoundingBox(
	const BoundingBox<double> & bbox);

} /* namespace */

} /* namespace */

#endif /* __INC_TEST_COMMON */
