/*
 * TestBoundingBox.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#include "geometry.h"

#include <gtest/gtest.h>

namespace {

using namespace dmscanlib;

TEST(TestBoundingBox, constructors) {
    Point<int> pt1(5, 10);
    Point<int> pt2(15, 20);

    BoundingBox<int> bbox(pt1, pt2);
    ASSERT_EQ(5, bbox.points[0].x);
    ASSERT_EQ(10, bbox.points[0].y);
    ASSERT_EQ(15, bbox.points[1].x);
    ASSERT_EQ(20, bbox.points[1].y);

    Point<int> pt3(12, 17);
    Point<int> pt4(17, 12);

    BoundingBox<int> bbox2(bbox);
    ASSERT_EQ(5, bbox2.points[0].x);
    ASSERT_EQ(10, bbox2.points[0].y);
    ASSERT_EQ(15, bbox2.points[1].x);
    ASSERT_EQ(20, bbox2.points[1].y);
}

TEST(TestBoundingBox, invalid) {
    Point<int> pt1(0, 0);
    ASSERT_THROW(BoundingBox<int> bbox(pt1, pt1), std::invalid_argument);

    // second point must always be greater thank first
    Point<int> pt2(-5, -10);
    ASSERT_THROW(BoundingBox<int> bbox(pt1, pt2), std::invalid_argument);
}

TEST(TestBoundingBox, translate) {
    Point<int> pt1(5, 10);
    Point<int> pt2(15, 20);

    BoundingBox<int> bbox(pt1, pt2);
    Point<int> tr(-5, -10);
    BoundingBox<int> bbox2(*bbox.translate(tr));
    ASSERT_EQ(0, bbox2.points[0].x);
    ASSERT_EQ(0, bbox2.points[0].y);
    ASSERT_EQ(10, bbox2.points[1].x);
    ASSERT_EQ(10, bbox2.points[1].y);
}

TEST(TestBoundingBox, rect) {
    Point<int> pt1(5, 10);
    Point<int> pt2(15, 20);
    Point<int> pt3(5, 20);
    Point<int> pt4(15, 10);

    Rect<int> rect(pt1, pt2, pt3, pt4);

    BoundingBox<int> bbox(*rect.getBoundingBox());
    ASSERT_EQ(5, bbox.points[0].x);
    ASSERT_EQ(10, bbox.points[0].y);
    ASSERT_EQ(15, bbox.points[1].x);
    ASSERT_EQ(20, bbox.points[1].y);
}

} /* namespace */
