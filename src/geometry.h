#ifndef __INC_STRUCTS_H
#define __INC_STRUCTS_H

#include <ostream>
#include <limits>
#include <algorithm>
#include <memory>

using namespace std;

template<typename T>
struct Point {
   const T x;
   const T y;

   Point(const T _x, const T _y) :
         x(_x), y(_y) {
   }

   Point(Point<T> & point) :
	   x(point.x), y(point.y)
   {

   }

   unique_ptr<Point<T> > scale(T factor) {
	   return new Point<T>(x * factor, y * factor);
   }
};

template<typename T>
struct Rect;

template<typename T>
struct BoundingBox {
   BoundingBox(const T x1, const T y1, const T x2, const T y2) :
         points( { Point<T>(x1, y1), Point<T>(x2, y2) }) {

   }

   BoundingBox(const BoundingBox<T> & r) :
	   points( {
	   Point<T>(r.points[0].x, r.points[0].y),
			   Point<T>(r.points[1].x, r.points[1].y)
   }) {

   }

   BoundingBox(const Rect<T> & rect) {
	   T minX = numeric_limits<T>::min();
	   T minY = numeric_limits<T>::min();

	   T maxX = numeric_limits<T>::max();
	   T maxY = numeric_limits<T>::max();

	   for (unsigned i = 0; i < 4; ++i) {
		   minX = min(minX, rect.corners[i].x);
		   minY = min(minY, rect.corners[i].y);
	   }
	   points.push_back(Point<T>(minX, minY));
	   points.push_back(Point<T>(maxX, maxY));
   }

   const vector<const Point<T>> points;
};

template<typename T>
ostream & operator<<(ostream &os, const Rect<T> & m) {
   os << "(" << m.corners[0].x << ", " << m.corners[0].y << "), " << "("
      << m.corners[1].x << ", " << m.corners[1].y << "), " << "("
      << m.corners[2].x << ", " << m.corners[2].y << "), " << "("
      << m.corners[3].x << ", " << m.corners[3].y << ")";
   return os;
}

template<typename T>
struct Rect {
	Rect(const Rect<T> &r) :
		corners({ r.corners[0], r.corners[1], r.corners[2], r.corners[3] })
	{

	}
	Rect(const Point<T> & pt1, const Point<T> & pt2, const Point<T> & pt3,
			const Point<T> & pt4) :
		corners({ pt1, pt2, pt3 pt4 })
	{

	}

	Rect(const T x1, const T y1, const T x2, const T y2, const T x3, const T y3, const T x4, const T y4) :
		corners( { Point<T>(x1, y1), Point<T>(x2, y2), Point<T>(x3, y3),
		Point<T>(x4, y4) }) {

	}

	Rect(const BoundingBox<T> bbox) :
		corners({
		Point<T>(bbox.points[0].x, bbox.points[0].y),
				Point<T>(bbox.points[0].x, bbox.points[1].y),
				Point<T>(bbox.points[1].x, bbox.points[1].y),
				Point<T>(bbox.points[1].x, bbox.points[0].y) })
	{

	}

	unique_ptr<Rect<T> > scale(const T factor) {
		vector<Point<T> > cornerVector;
		for (unsigned i = 0; i < 4; ++i) {
			cornerVector.push_back(corners[i].scale(factor));
		}
		return new Rect<T>()
	}

	void translate(const Point<T> & distance) {
		for (unsigned i = 0; i < 4; ++i) {
			corners[i].x += distance.x;
			corners[i].y += distance.y;
		}
	}

	const Point<T> corners[4];
   friend ostream & operator<<<>(std::ostream & os, const Rect<T> & m);
};

#endif /* __INC_STRUCTS_H */

