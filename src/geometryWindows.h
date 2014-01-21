#ifndef __INC_GEOMETRY_WINDOWS_H
#define __INC_GEOMETRY_WINDOWS_H

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

#include <ostream>
#include <limits>
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace dmscanlib {

template<typename T>
std::ostream & operator<<(std::ostream &os, const cv::Point_<T> & m) {
    os << "(" << m.x << ", " << m.y << ")";
    return os;
}

template<typename T>
struct Rect;

template<typename T>
struct BoundingBox {
    BoundingBox(const cv::Point_<T> & p1, const cv::Point_<T> & p2) {
        points[0] = p1;
        points[1] = p2;

        if (!isValid()) {
            throw std::invalid_argument("invalid bounding box");
        }

    }

    BoundingBox(const BoundingBox<T> & that) {
        points[0] = that.points[0];
        points[1] = that.points[1];

        if (!isValid()) {
            throw std::invalid_argument("invalid bounding box");
        }

    }

    bool isValid() {
        return (points[0].x < points[1].x)
                && (points[0].y < points[1].y);
    }

    T getWidth() const {
        return points[1].x - points[0].x;
    }

    T getHeight() const {
        return points[1].y - points[0].y;
    }

    std::unique_ptr<const BoundingBox<T> > translate(const cv::Point_<T> & distance) const {
        std::unique_ptr<const cv::Point_<T> > pt1 = std::move(points[0].translate(distance));
        std::unique_ptr<const cv::Point_<T> > pt2 = std::move(points[1].translate(distance));
        return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(*pt1, *pt2));
    }

    cv::Point_<T> points[2];
};

template<typename T>
struct ScanRegion {
    ScanRegion(const cv::Point_<T> & p1, const cv::Point_<T> & p2) {
        points[0] = p1;
        points[1] = p2;

        if (!isValid()) {
            throw std::invalid_argument("invalid bounding box");
        }

    }

    ScanRegion(const ScanRegion<T> & that) {
        points[0] = that.points[0];
        points[1] = that.points[1];

        if (!isValid()) {
            throw std::invalid_argument("invalid bounding box");
        }
    }

    ScanRegion(const BoundingBox<T> & bbox) {
        points[0] = bbox.points[0];
        points[1] = bbox.points[1];

        if (!isValid()) {
            throw std::invalid_argument("invalid scan region");
        }
    }

    bool isValid() {
        return (points[0].x >= 0) && (points[1].x >= 0)
                && (points[0].y >= 0) && (points[1].y >= 0);
    }

    T getWidth() const {
        return points[1].x - points[0].x;
    }

    T getHeight() const {
        return points[1].y - points[0].y;
    }

    // WIA regions are not bounding boxes
    std::unique_ptr<const BoundingBox<T> > toBoundingBox() const {
        if ((points[1].x < points[0].x) || (points[1].y < points[0].y)) {
            return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(
                    points[0], *points[1].translate(points[0])));
        }

        return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(points[0], points[1]));
    }

    std::unique_ptr<const ScanRegion<T> > translate(const cv::Point_<T> & distance) const {
        std::unique_ptr<const cv::Point_<T> > pt1 = std::move(points[0].translate(distance));
        std::unique_ptr<const cv::Point_<T> > pt2 = std::move(points[1].translate(distance));
        return std::unique_ptr<const ScanRegion<T> >(new ScanRegion<T>(*pt1, *pt2));
    }

    cv::Point_<T> points[2];
};

template<typename T>
std::ostream & operator<<(std::ostream &os, const BoundingBox<T> & m) {
    os << "(" << m.points[0].x << ", " << m.points[0].y << "), " << "("
            << m.points[1].x << ", " << m.points[1].y << ")";
    return os;
}

template<typename T>
std::ostream & operator<<(std::ostream &os, const ScanRegion<T> & m) {
    os << "(" << m.points[0].x << ", " << m.points[0].y << "), " << "("
            << m.points[1].x << ", " << m.points[1].y << ")";
    return os;
}

template<typename T>
std::ostream & operator<<(std::ostream &os, const Rect<T> & m) {
    os << "(" << m.corners[0].x << ", " << m.corners[0].y << "), " << "("
            << m.corners[1].x << ", " << m.corners[1].y << "), " << "("
            << m.corners[2].x << ", " << m.corners[2].y << "), " << "("
            << m.corners[3].x << ", " << m.corners[3].y << ")";
    return os;
}

template<typename T>
struct Rect {
    Rect(const Rect<T> & that) {
        corners[0] = that.corners[0];
        corners[1] = that.corners[1];
        corners[2] = that.corners[2];
        corners[3] = that.corners[3];
    }

    Rect(const cv::Point_<T> & pt1, const cv::Point_<T> & pt2, const cv::Point_<T> & pt3,
            const cv::Point_<T> & pt4) {
        corners[0] = pt1;
        corners[1] = pt2;
        corners[2] = pt3;
        corners[3] = pt4;

    }

    Rect(const BoundingBox<T> & bbox) {
        corners[0].x = bbox.points[0].x;
        corners[0].y = bbox.points[0].y;
        corners[1].x = bbox.points[0].x;
        corners[1].y = bbox.points[1].y;
        corners[2].x = bbox.points[1].x;
        corners[2].y = bbox.points[1].y;
        corners[3].x = bbox.points[1].x;
        corners[3].y = bbox.points[0].y;
    }

    std::unique_ptr<const Rect<T> > scale(const T factor) const {
        std::unique_ptr<const cv::Point_<T> > pt1 = std::move(corners[0].scale(factor));
        std::unique_ptr<const cv::Point_<T> > pt2 = std::move(corners[1].scale(factor));
        std::unique_ptr<const cv::Point_<T> > pt3 = std::move(corners[2].scale(factor));
        std::unique_ptr<const cv::Point_<T> > pt4 = std::move(corners[3].scale(factor));
        return std::unique_ptr<const Rect<T> >(new Rect<T>(*pt1, *pt2, *pt3, *pt4));
    }

    std::unique_ptr<const Rect<T> > translate(const cv::Point_<T> & distance) const {
        std::unique_ptr<const cv::Point_<T> > pt1 = std::move(corners[0].translate(distance));
        std::unique_ptr<const cv::Point_<T> > pt2 = std::move(corners[1].translate(distance));
        std::unique_ptr<const cv::Point_<T> > pt3 = std::move(corners[2].translate(distance));
        std::unique_ptr<const cv::Point_<T> > pt4 = std::move(corners[3].translate(distance));
        return std::unique_ptr<const Rect<T> >(new Rect<T>(*pt1, *pt2, *pt3, *pt4));
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

    cv::Point_<T> corners[4];
    friend std::ostream & operator<<<>(std::ostream & os, const Rect<T> & m);
};

} /* namespace */

#endif /* __INC_GEOMETRY_WINDOWS_H */

