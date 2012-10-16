#ifndef __INC_STRUCTS_H
#define __INC_STRUCTS_H

#include <ostream>
#include <limits>
#include <algorithm>

using namespace std;

template<typename T>
struct Point {
	T x;
	T y;

	Point(T _x, T _y) :
			x(_x), y(_y) {
	}
	Point() {
	}
};

template<typename T>
struct Rect;

template<typename T>
ostream & operator<<(ostream &os, const Rect<T> & m) {
	os << "(" << m.corners[0].x << ", " << m.corners[0].y << "), " << "("
			<< m.corners[1].x << ", " << m.corners[1].y << "), " << "("
			<< m.corners[2].x << ", " << m.corners[2].y << "), " << "("
			<< m.corners[3].x << ", " << m.corners[3].y << ")";
	return os;
}

template<typename T>
struct BoundingBox {
	BoundingBox(T x1, T y1, T x2, T y2) :
			points( { Point<T>(x1, y1), Point<T>(x2, y2) }) {

	}

	BoundingBox(const BoundingBox<T> & r) :
			points( { Point<T>(r.points[0].x, r.points[0].y),
		Point<T>(r.points[1].x, r.points[1].y) }) {

	}

	BoundingBox() :
			points( { Point<T>(), Point<T>() }) {

	}

	BoundingBox(const Rect<T> & rect) {
		points[0].x = numeric_limits<T>::max();
		points[0].y = numeric_limits<T>::max();

		points[1].x = numeric_limits<T>::min();
		points[1].y = numeric_limits<T>::min();

		for (unsigned i = 0; i < 4; ++i) {
			points[0].x = min(points[0].x, rect.corners[i].x);
			points[0].y = min(points[0].y, rect.corners[i].y);

			points[1].x = max(points[1].x, rect.corners[i].x);
			points[1].y = max(points[1].y, rect.corners[i].y);
		}

	}

	Point<T> points[2];
};

template<typename T>
struct Rect {
	Rect(Rect<T> &r) :
		corners({ r.corners[0], r.corners[1], r.corners[2], r.corners[3] })
	{

	}

	Rect(T x1, T y1, T x2, T y2, T x3, T y3, T x4, T y4) :
			corners( { Point<T>(x1, y1), Point<T>(x2, y2), Point<T>(x3, y3),
					Point<T>(x4, y4) }) {

	}

	Rect(BoundingBox<T> bbox) :
			corners(
					{ Point<T>(bbox.points[0].x, bbox.points[0].y),
		Point<T>(bbox.points[0].x, bbox.points[1].y),
		Point<T>(bbox.points[1].x, bbox.points[1].y),
		Point<T>(bbox.points[1].x, bbox.points[0].y) })
	{

	}

	Rect<T> & operator=(const Rect<T> & rhs) {
		for (unsigned i = 0; i < 4; ++i) {
			corners[i].x = rhs.corners[i].x;
			corners[i].y = rhs.corners[i].y;
		}
		return *this;
	}

	void scale(T factor) {
		for (unsigned i = 0; i < 4; ++i) {
			corners[i].x *= factor;
			corners[i].y *= factor;
		}
	}

	void translate(Point<T> & distance) {
		for (unsigned i = 0; i < 4; ++i) {
			corners[i].x += distance.x;
			corners[i].y += distance.y;
		}
	}

	Point<T> corners[4];
	friend ostream & operator<<<>(std::ostream & os, const Rect<T> & m);
};

#endif /* __INC_STRUCTS_H */

