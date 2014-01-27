/*
 * TestWellRectangle.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "decoder/WellRectangle.h"

#include <gtest/gtest.h>

namespace {

using namespace dmscanlib;

TEST(TestWellRectangle, getRectangle) {
    WellRectangle wr("label", 5, 5, 20, 20);
}

} /* namespace */
