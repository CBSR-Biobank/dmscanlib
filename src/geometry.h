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

	BoundingBox() :
			points( { Point<T>(), Point<T>() }) {

	}

	Point<T> points[2];
};

template<typename T>
struct Rect {
	Rect(T x1, T y1, T x2, T y2, T x3, T y3, T x4, T y4) :
			corners( { Point<T>(x1, y1), Point<T>(x2, y2), Point<T>(x3, y3),
					Point<T>(x4, y4) }) {

	}

	Rect(BoundingBox<T> bbox) :
			corners(
					{ Point<T>(bbox.point[0].x, bbox.point[0].y), Point<T>(
							bbox.point[0].x, bbox.point[1].y), Point<T>(
							bbox.point[1].x, bbox.point[1].y), Point<T>(
							bbox.point[1].x, bbox.point[0].y) }) {

	}

	Point<T> corners[4];
	friend ostream & operator<<<>(std::ostream & os, const Rect<T> & m);
};

template<typename T>
void Rect2BoundingBox(const Rect<T> & rect, BoundingBox<T> & bbox) {
	bbox.points[0].x = numeric_limits<T>::max();
	bbox.points[0].y = numeric_limits<T>::max();

	bbox.points[1].x = numeric_limits<T>::min();
	bbox.points[1].y = numeric_limits<T>::min();

	for (unsigned i = 0; i < 4; ++i) {
		bbox.points[0].x = min(bbox.points[0].x, rect.corners[i].x);
		bbox.points[0].y = min(bbox.points[0].y, rect.corners[i].y);

		bbox.points[1].x = max(bbox.points[1].x, rect.corners[i].x);
		bbox.points[1].y = max(bbox.points[1].y, rect.corners[i].y);
	}

}

#endif /* __INC_STRUCTS_H */

