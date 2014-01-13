/*
 * TestPoint.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#include "geometry.h"

#include <gtest/gtest.h>

namespace {

TEST(TestPoint, Constructors) {
    dmscanlib::Point<int> pt(5, 10);
    ASSERT_EQ(5, pt.x);
    ASSERT_EQ(10, pt.y);

    dmscanlib::Point<int> pt2(pt);
    ASSERT_EQ(5, pt2.x);
    ASSERT_EQ(10, pt2.y);
}

TEST(TestPoint, scale) {
    dmscanlib::Point<int> pt(5, 10);
    dmscanlib::Point<int> pt2(*pt.scale(-1));
    ASSERT_EQ(-5, pt2.x);
    ASSERT_EQ(-10, pt2.y);
}

TEST(TestPoint, translate) {
    dmscanlib::Point<int> pt1(5, 10);
    dmscanlib::Point<int> pt2(-5, -10);
    dmscanlib::Point<int> pt_t(*pt1.translate(pt2));
    ASSERT_EQ(0, pt_t.x);
    ASSERT_EQ(0, pt_t.y);
}

} /* namespace */
