#ifndef __INC_GEOMETRY_LINUX_H
#define __INC_GEOMETRY_LINUX_H

#include <ostream>
#include <limits>
#include <algorithm>
#include <memory>
#include <glog/logging.h>
#include <stdexcept>

namespace dmscanlib {

template<typename T>
struct Point {
	const T x;
	const T y;

	Point(const T _x, const T _y) :
		x(_x), y(_y) {
	}

	Point(const Point<T> & point) :
		x(point.x), y(point.y)
	{

	}

	std::unique_ptr<const Point<T> > scale(const T factor) const {
		return std::unique_ptr<const Point<T> >(new Point<T>(x * factor, y * factor));
	}

	std::unique_ptr<const Point<T> > translate(const Point<T> distance) const {
		return std::unique_ptr<const Point<T> >(new Point<T>(x + distance.x, y + distance.y));
	}
};

template<typename T>
std::ostream & operator<<(std::ostream &os, const Point<T> & m) {
	os << "(" << m.x << ", " << m.y << ")";
	return os;
}

template<typename T>
struct Rect;

template<typename T>
struct BoundingBox {
	BoundingBox(const Point<T> & p1, const Point<T> & p2) :
		points( { p1, p2 } )
	{
		if (!isValid()) {
			throw std::invalid_argument("invalid bounding box");
		}

	}

	BoundingBox(const BoundingBox<T> & that) :
		points( {
		Point<T>(that.points[0].x, that.points[0].y),
				Point<T>(that.points[1].x, that.points[1].y)
	}) {
		if (!isValid()) {
			throw std::invalid_argument("invalid bounding box");
		}

	}

	virtual ~BoundingBox() { }

	bool isValid() {
		return (points[0].x < points[1].x) && (points[0].y < points[1].y);
	}

	T getWidth() const {
		return points[1].x - points[0].x;
	}

	T getHeight() const {
		return points[1].y - points[0].y;
	}

	std::unique_ptr<const BoundingBox<T> > translate(const Point<T> & distance) const {
		std::unique_ptr<const Point<T> > pt1 = std::move(points[0].translate(distance));
		std::unique_ptr<const Point<T> > pt2 = std::move(points[1].translate(distance));
		return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(*pt1, *pt2));
	}

	const Point<T> points[2];
};

template<typename T>
struct ScanRegion {
	ScanRegion(const Point<T> & p1, const Point<T> & p2) : points( { p1, p2 } )
	{
		if (!isValid()) {
			throw std::invalid_argument("invalid scan region");
		}
	}

	ScanRegion(const ScanRegion<T> & that) : points( {
		Point<T>(that.points[0].x, that.points[0].y),
				Point<T>(that.points[1].x, that.points[1].y)
	}) {
		if (!isValid()) {
			throw std::invalid_argument("invalid scan region");
		}
	}

	ScanRegion(const BoundingBox<T> & bbox) : points( { bbox.points[0], bbox.points[1] } )
	{
		if (!isValid()) {
			throw std::invalid_argument("invalid scan region");
		}
	}

	virtual ~ScanRegion() {}

	bool isValid() {
		return (points[0].x >= 0) && (points[1].x >= 0)
				&& (points[0].y >= 0) && (points[1].y >= 0);
	}

	// WIA regions are not bounding boxes
	std::unique_ptr<const BoundingBox<T> > toBoundingBox() const {
		if ((points[1].x < points[0].x) || (points[1].y < points[0].y)) {
			return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(
					points[0], points[1].translate(points[0])));
		}

		return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(
				points[0], points[1]));
	}

	std::unique_ptr<const ScanRegion<T> > translate(const Point<T> & distance) const {
		std::unique_ptr<const Point<T> > pt1 = std::move(points[0].translate(distance));
		std::unique_ptr<const Point<T> > pt2 = std::move(points[1].translate(distance));
		return std::unique_ptr<const ScanRegion<T> >(new ScanRegion<T>(*pt1, *pt2));
	}

	const Point<T> points[2];
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
	os << m.corners[0] << ", " << m.corners[1]<< m.corners[2] << ", " << m.corners[3];
	return os;
}

template<typename T>
struct Rect {
	Rect(const Point<T> & pt1, const Point<T> & pt2, const Point<T> & pt3,
			const Point<T> & pt4) :
				corners({ pt1, pt2, pt3, pt4 })
	{

	}

	Rect(const Rect<T> & that) :
		corners({  that.corners[0],  that.corners[1],  that.corners[2],  that.corners[3] })
	{

	}

	// goes clockwise through points starting at top left
	Rect(const BoundingBox<T> & bbox) :
		corners({
		Point<T>(bbox.points[0].x, bbox.points[0].y),
				Point<T>(bbox.points[0].x, bbox.points[1].y),
				Point<T>(bbox.points[1].x, bbox.points[1].y),
				Point<T>(bbox.points[1].x, bbox.points[0].y) })
	{
	}

	std::unique_ptr<const Rect<T> > scale(const T factor) const {
		std::unique_ptr<const Point<T> > pt1 = std::move(corners[0].scale(factor));
		std::unique_ptr<const Point<T> > pt2 = std::move(corners[1].scale(factor));
		std::unique_ptr<const Point<T> > pt3 = std::move(corners[2].scale(factor));
		std::unique_ptr<const Point<T> > pt4 = std::move(corners[3].scale(factor));
		return std::unique_ptr<const Rect<T> >(new Rect<T>(*pt1, *pt2, *pt3, *pt4));
	}

	std::unique_ptr<const Rect<T> > translate(const Point<T> & distance) const {
		std::unique_ptr<const Point<T> > pt1 = std::move(corners[0].translate(distance));
		std::unique_ptr<const Point<T> > pt2 = std::move(corners[1].translate(distance));
		std::unique_ptr<const Point<T> > pt3 = std::move(corners[2].translate(distance));
		std::unique_ptr<const Point<T> > pt4 = std::move(corners[3].translate(distance));
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
		Point<T> p1(minX, minY);
		Point<T> p2(maxX, maxY);
		return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(p1, p2));
	}

	const Point<T> corners[4];
	friend std::ostream & operator<<<>(std::ostream & os, const Rect<T> & m);
};

} /* namespace */

#endif /* __INC_GEOMETRY_LINUX_H */

