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

TEST(TestRect, constructors) {
    Point<int> pt1(5, 10);
    Point<int> pt2(15, 20);
    Point<int> pt3(5, 20);
    Point<int> pt4(15, 10);

    Rect<int> rect(pt1, pt2, pt3, pt4);
    ASSERT_EQ(5, rect.corners[0].x);
    ASSERT_EQ(10, rect.corners[0].y);
    ASSERT_EQ(15, rect.corners[1].x);
    ASSERT_EQ(20, rect.corners[1].y);
    ASSERT_EQ(5, rect.corners[2].x);
    ASSERT_EQ(20, rect.corners[2].y);
    ASSERT_EQ(15, rect.corners[3].x);
    ASSERT_EQ(10, rect.corners[3].y);

    Rect<int> rect2(rect);
    ASSERT_EQ(5, rect2.corners[0].x);
    ASSERT_EQ(10, rect2.corners[0].y);
    ASSERT_EQ(15, rect2.corners[1].x);
    ASSERT_EQ(20, rect2.corners[1].y);
    ASSERT_EQ(5, rect2.corners[2].x);
    ASSERT_EQ(20, rect2.corners[2].y);
    ASSERT_EQ(15, rect2.corners[3].x);
    ASSERT_EQ(10, rect2.corners[3].y);

    BoundingBox<int> bbox(pt1, pt2);
    Rect<int> rect3(bbox);
    ASSERT_EQ(5, rect3.corners[0].x);
    ASSERT_EQ(10, rect3.corners[0].y);
    ASSERT_EQ(5, rect3.corners[1].x);
    ASSERT_EQ(20, rect3.corners[1].y);
    ASSERT_EQ(15, rect3.corners[2].x);
    ASSERT_EQ(20, rect3.corners[2].y);
    ASSERT_EQ(15, rect3.corners[3].x);
    ASSERT_EQ(10, rect3.corners[3].y);
}

TEST(TestRect, scale) {
    Point<int> pt1(3, 3);
    Point<int> pt2(6, 6);

    BoundingBox<int> bbox(pt1, pt2);
    Rect<int> rect1(bbox);
    Rect<int> rect2(*rect1.scale(2));

    ASSERT_EQ(6, rect2.corners[0].x);
    ASSERT_EQ(6, rect2.corners[0].y);
    ASSERT_EQ(6, rect2.corners[1].x);
    ASSERT_EQ(12, rect2.corners[1].y);
    ASSERT_EQ(12, rect2.corners[2].x);
    ASSERT_EQ(12, rect2.corners[2].y);
    ASSERT_EQ(12, rect2.corners[3].x);
    ASSERT_EQ(6, rect2.corners[3].y);
}

TEST(TestRect, translate) {
    Point<int> pt1(3, 3);
    Point<int> pt2(6, 6);

    BoundingBox<int> bbox(pt1, pt2);
    Rect<int> rect1(bbox);
    Point<int> tr(10, 10);
    Rect<int> rect2(*rect1.translate(tr));

    ASSERT_EQ(13, rect2.corners[0].x);
    ASSERT_EQ(13, rect2.corners[0].y);
    ASSERT_EQ(13, rect2.corners[1].x);
    ASSERT_EQ(16, rect2.corners[1].y);
    ASSERT_EQ(16, rect2.corners[2].x);
    ASSERT_EQ(16, rect2.corners[2].y);
    ASSERT_EQ(16, rect2.corners[3].x);
    ASSERT_EQ(13, rect2.corners[3].y);
}

} /* namespace */
