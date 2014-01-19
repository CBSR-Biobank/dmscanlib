/*
 * TestScanRegion.cpp
 *
 *  Created on: 2013-02-01
 *      Author: nelson
 */

#include "geometry.h"

#include <gtest/gtest.h>

namespace {

using namespace dmscanlib;

TEST(TestScanRegion, constructors) {
    Point<int> pt1(5, 10);
    Point<int> pt2(15, 20);

    ScanRegion<int> bbox(pt1, pt2);
    ASSERT_EQ(5, bbox.points[0].x);
    ASSERT_EQ(10, bbox.points[0].y);
    ASSERT_EQ(15, bbox.points[1].x);
    ASSERT_EQ(20, bbox.points[1].y);

    Point<int> pt3(12, 17);
    Point<int> pt4(17, 12);

    ScanRegion<int> bbox2(bbox);
    ASSERT_EQ(5, bbox2.points[0].x);
    ASSERT_EQ(10, bbox2.points[0].y);
    ASSERT_EQ(15, bbox2.points[1].x);
    ASSERT_EQ(20, bbox2.points[1].y);
}

TEST(TestScanRegion, valid) {
    // all points must be positive
    Point<int> pt1(0, 0);
    Point<int> pt2(-5, -10);
    ASSERT_THROW(ScanRegion<int> scanRegion(pt1, pt2), std::invalid_argument);
    ASSERT_THROW(ScanRegion<int> scanRegion(pt2, pt1), std::invalid_argument);

    // second point smaller than first - valid for scan region
    Point<int> pt3(5, 5);
    Point<int> pt4(3, 3);
    ScanRegion<int> scanRegion(pt3, pt4);
}

TEST(TestScanRegion, translate) {
    Point<int> pt1(5, 10);
    Point<int> pt2(15, 20);

    ScanRegion<int> scanRegion(pt1, pt2);
    Point<int> tr(-5, -10);
    ScanRegion<int> scanRegion2(*scanRegion.translate(tr));
    ASSERT_EQ(0, scanRegion2.points[0].x);
    ASSERT_EQ(0, scanRegion2.points[0].y);
    ASSERT_EQ(10, scanRegion2.points[1].x);
    ASSERT_EQ(10, scanRegion2.points[1].y);
}

TEST(TestScanRegion, rect) {
    Point<int> pt1(5, 10);
    Point<int> pt2(15, 20);
    Point<int> pt3(5, 20);
    Point<int> pt4(15, 10);

    Rect<int> rect(pt1, pt2, pt3, pt4);

    ScanRegion<int> scanRegion(*rect.getBoundingBox());
    ASSERT_EQ(5, scanRegion.points[0].x);
    ASSERT_EQ(10, scanRegion.points[0].y);
    ASSERT_EQ(15, scanRegion.points[1].x);
    ASSERT_EQ(20, scanRegion.points[1].y);
}

} /* namespace */
