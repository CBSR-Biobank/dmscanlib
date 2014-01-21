#ifndef __INC_GEOMETRY_LINUX_H
#define __INC_GEOMETRY_LINUX_H

#include <ostream>
#include <limits>
#include <algorithm>
#include <memory>
#include <glog/logging.h>
#include <stdexcept>

#include <opencv/cv.h>

namespace dmscanlib {

template<typename T>
struct Rect;

template<typename T>
struct BoundingBox {
    BoundingBox(const cv::Point_<T> & p1, const cv::Point_<T> & p2) :
            points( { p1, p2 })
    {
        if (!isValid()) {
            throw std::invalid_argument("invalid bounding box");
        }

    }

    BoundingBox(const BoundingBox<T> & that) :
            points( {
                    cv::Point_<T>(that.points[0].x, that.points[0].y),
                    cv::Point_<T>(that.points[1].x, that.points[1].y)
            }) {
        if (!isValid()) {
            throw std::invalid_argument("invalid bounding box");
        }

    }

    virtual ~BoundingBox() {
    }

    bool isValid() {
        return (points[0].x < points[1].x) && (points[0].y < points[1].y);
    }

    T getWidth() const {
        return points[1].x - points[0].x;
    }

    T getHeight() const {
        return points[1].y - points[0].y;
    }

    std::unique_ptr<const BoundingBox<T> > translate(const cv::Point_<T> & distance) const {
        const cv::Point_<T> pt1 = std::move(points[0].translate(distance));
        const cv::Point_<T> pt2 = std::move(points[1].translate(distance));
        return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(*pt1, *pt2));
    }

    const cv::Point_<T> points[2];
};

template<typename T>
struct ScanRegion {
    ScanRegion(const cv::Point_<T> & p1, const cv::Point_<T> & p2) :
            points( { p1, p2 })
    {
        if (!isValid()) {
            throw std::invalid_argument("invalid scan region");
        }
    }

    ScanRegion(const ScanRegion<T> & that) :
            points( {
                    cv::Point_<T>(that.points[0].x, that.points[0].y),
                    cv::Point_<T>(that.points[1].x, that.points[1].y)
            }) {
        if (!isValid()) {
            throw std::invalid_argument("invalid scan region");
        }
    }

    ScanRegion(const BoundingBox<T> & bbox) :
            points( { bbox.points[0], bbox.points[1] })
    {
        if (!isValid()) {
            throw std::invalid_argument("invalid scan region");
        }
    }

    virtual ~ScanRegion() {
    }

    bool isValid() {
        return (points[0].x >= 0) && (points[1].x >= 0)
                && (points[0].y >= 0) && (points[1].y >= 0);
    }

    // WIA regions are not bounding boxes
    std::unique_ptr<const BoundingBox<T> > toBoundingBox() const {
        if ((points[1].x < points[0].x) || (points[1].y < points[0].y)) {
            return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(
                    points[0], *points[1].translate(points[0])));
        }

        return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(
                points[0], points[1]));
    }

    std::unique_ptr<const ScanRegion<T> > translate(const cv::Point_<T> & distance) const {
        const cv::Point_<T> pt1(points[0].x + distance, points[0].y + distance);
        const cv::Point_<T> pt2(points[1].x + distance, points[1].y + distance);
        return std::unique_ptr<const ScanRegion<T> >(new ScanRegion<T>(pt1, pt2));
    }

    const cv::Point_<T> points[2];
};

template<typename T>
std::ostream & operator<<(std::ostream &os, const BoundingBox<T> & m) {
    os << m.points[0] << ", " << m.points[1];
    return os;
}

template<typename T>
std::ostream & operator<<(std::ostream &os, const ScanRegion<T> & m) {
    os << m.points[0] << ", " << m.points[1];
    return os;
}

template<typename T>
std::ostream & operator<<(std::ostream &os, const Rect<T> & m) {
    os << m.corners[0] << ", " << m.corners[1] << m.corners[2] << ", " << m.corners[3];
    return os;
}

template<typename T>
struct Rect {
    Rect(const cv::Point_<T> & pt1, const cv::Point_<T> & pt2, const cv::Point_<T> & pt3,
            const cv::Point_<T> & pt4) :
            corners( { pt1, pt2, pt3, pt4 })
    {

    }

    Rect(const Rect<T> & that) :
            corners( { that.corners[0], that.corners[1], that.corners[2], that.corners[3] })
    {

    }

    // goes clockwise through points starting at top left
    Rect(const BoundingBox<T> & bbox) :
            corners( {
                    cv::Point_<T>(bbox.points[0].x, bbox.points[0].y),
                    cv::Point_<T>(bbox.points[0].x, bbox.points[1].y),
                    cv::Point_<T>(bbox.points[1].x, bbox.points[1].y),
                    cv::Point_<T>(bbox.points[1].x, bbox.points[0].y) })
    {
    }

    std::unique_ptr<const Rect<T> > scale(const T factor) const {
        const cv::Point_<T> pt1(
                corners[0].x * factor, corners[0].y * factor);
        const cv::Point_<T> pt2(
                corners[1].x * factor, corners[1].y * factor);
        const cv::Point_<T> pt3(
                corners[2].x * factor, corners[2].y * factor);
        const cv::Point_<T> pt4(
                corners[3].x * factor, corners[3].y * factor);
        return std::unique_ptr<const Rect<T> >(new Rect<T>(pt1, pt2, pt3, pt4));
    }

    std::unique_ptr<const Rect<T> > translate(const cv::Point_<T> & distance) const {
        const cv::Point_<T> pt1(
                corners[0].x + distance.x, corners[0].y + distance.y);
        const cv::Point_<T> pt2(
                corners[1].x + distance.x, corners[1].y + distance.y);
        const cv::Point_<T> pt3(
                corners[2].x + distance.x, corners[2].y + distance.y);
        const cv::Point_<T> pt4(
                corners[3].x + distance.x, corners[3].y + distance.y);
        return std::unique_ptr<const Rect<T> >(new Rect<T>(pt1, pt2, pt3, pt4));
    }

    std::unique_ptr<const BoundingBox<T> > getBoundingBox() const {
        T maxX = std::numeric_limits<T>::min();
        T maxY = std::numeric_limits<T>::min();

        T minX = std::numeric_limits<T>::max();
        T minY = std::numeric_limits<T>::max();

        for (unsigned i = 0; i < 4; ++i) {
            minX = std::min(minX, corners[i].x);
            minY = std::min(minY, corners[i].y);

            maxX = std::max(maxX, corners[i].x);
            maxY = std::max(maxY, corners[i].y);
        }
        cv::Point_<T> p1(minX, minY);
        cv::Point_<T> p2(maxX, maxY);
        return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(p1, p2));
    }

    const cv::Point_<T> corners[4];
    friend std::ostream & operator<<<>(std::ostream & os, const Rect<T> & m);
};

} /* namespace */

#endif /* __INC_GEOMETRY_LINUX_H */

