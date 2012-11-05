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
struct Point {
   T x;
   T y; 

   Point() {
   }

   Point(const T _x, const T _y) : x(_x), y(_y) {
   }

   Point(const Point<T> & point)
   {
	   x = point.x;
	   y = point.y;

   }

   Point<T> & operator= (const Point<T> & that) {
	   x = that.x;
	   y = that.y;
	   return *this;
   }

   std::unique_ptr<const Point<T> > scale(const T factor) const {
      return std::unique_ptr<const Point<T> >(new Point<T>(x * factor, y * factor));
   }

   std::unique_ptr<const Point<T> > translate(const Point<T> distance) const {
      return std::unique_ptr<const Point<T> >(new Point<T>(x + distance.x, y + distance.y));
   }
};

template<typename T>
struct Rect;

template<typename T>
struct BoundingBox {
   BoundingBox(const Point<T> & p1, const Point<T> & p2) {
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

   std::unique_ptr<const BoundingBox<T> > translate(const Point<T> & distance) const {
      std::unique_ptr<const Point<T> > pt1 = std::move(points[0].translate(distance));
      std::unique_ptr<const Point<T> > pt2 = std::move(points[1].translate(distance));
      return std::unique_ptr<const BoundingBox<T> >(new BoundingBox<T>(*pt1, *pt2));
   }

   Point<T> points[2];
};

template<typename T>
std::ostream & operator<<(std::ostream &os, const BoundingBox<T> & m) {
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

   Rect(const Point<T> & pt1, const Point<T> & pt2, const Point<T> & pt3,
        const Point<T> & pt4) {
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

   Point<T> corners[4];
   friend std::ostream & operator<<<>(std::ostream & os, const Rect<T> & m);
};

} /* namespace */

#endif /* __INC_GEOMETRY_WINDOWS_H */

