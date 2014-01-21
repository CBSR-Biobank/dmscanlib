/*
 * TestWellRectangle.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#include "decoder/WellRectangle.h"

#include <gtest/gtest.h>

namespace {

using namespace dmscanlib;

TEST(TestWellRectangle, constructors) {
    cv::Point_<float> pt1(5, 10);
    cv::Point_<float> pt2(15, 20);
    cv::Point_<float> pt3(5, 20);
    cv::Point_<float> pt4(15, 10);
    Rect<float> rect(pt1, pt2, pt3, pt4);

    WellRectangle<float> wr("label", rect);

    ASSERT_EQ("label", wr.getLabel());

    ASSERT_EQ(5, wr.getCornerX(0));
    ASSERT_EQ(10, wr.getCornerY(0));

    ASSERT_EQ(15, wr.getCornerX(1));
    ASSERT_EQ(20, wr.getCornerY(1));

    ASSERT_EQ(5, wr.getCornerX(2));
    ASSERT_EQ(20, wr.getCornerY(2));

    ASSERT_EQ(15, wr.getCornerX(3));
    ASSERT_EQ(10, wr.getCornerY(3));

    BoundingBox<float> bbox(pt1, pt2);
    WellRectangle<float> wr2("label2", bbox);

    ASSERT_EQ("label2", wr2.getLabel());

    ASSERT_EQ(5, wr2.getCornerX(0));
    ASSERT_EQ(10, wr2.getCornerY(0));

    ASSERT_EQ(5, wr2.getCornerX(1));
    ASSERT_EQ(20, wr2.getCornerY(1));

    ASSERT_EQ(15, wr2.getCornerX(2));
    ASSERT_EQ(20, wr2.getCornerY(2));

    ASSERT_EQ(15, wr2.getCornerX(3));
    ASSERT_EQ(10, wr2.getCornerY(3));
}

TEST(TestWellRectangle, getRectanglePoints) {
    cv::Point_<float> pt1(5, 10);
    cv::Point_<float> pt2(15, 20);
    cv::Point_<float> pt3(5, 20);
    cv::Point_<float> pt4(15, 10);
    Rect<float> rect(pt1, pt2, pt3, pt4);

    WellRectangle<float> wr("label", rect);

    const cv::Point_<float> p1 = wr.getCorner(0);
    ASSERT_EQ(5, p1.x);
    ASSERT_EQ(10, p1.y);

    const cv::Point_<float> p2 = wr.getCorner(1);
    ASSERT_EQ(15, p2.x);
    ASSERT_EQ(20, p2.y);

    const cv::Point_<float> p3 = wr.getCorner(2);
    ASSERT_EQ(5, p3.x);
    ASSERT_EQ(20, p3.y);

    const cv::Point_<float> p4 = wr.getCorner(3);
    ASSERT_EQ(15, p4.x);
    ASSERT_EQ(10, p4.y);
}

TEST(TestWellRectangle, getRectangle) {
    cv::Point_<float> pt1(5, 10);
    cv::Point_<float> pt2(15, 20);
    cv::Point_<float> pt3(5, 20);
    cv::Point_<float> pt4(15, 10);
    Rect<float> rect(pt1, pt2, pt3, pt4);

    WellRectangle<float> wr("label", rect);

    const Rect<float> & r = wr.getRectangle();

    ASSERT_EQ(5, r.corners[0].x);
    ASSERT_EQ(5, r.corners[0].x);
    ASSERT_EQ(10, r.corners[0].y);
    ASSERT_EQ(15, r.corners[1].x);
    ASSERT_EQ(20, r.corners[1].y);
    ASSERT_EQ(5, r.corners[2].x);
    ASSERT_EQ(20, r.corners[2].y);
    ASSERT_EQ(15, r.corners[3].x);
    ASSERT_EQ(10, r.corners[3].y);
}

} /* namespace */
