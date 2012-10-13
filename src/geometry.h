#ifndef __INC_STRUCTS_H
#define __INC_STRUCTS_H

#include <string>

using namespace std;

template <typename T>
struct Point {
   T x;
   T y;

   Point (T _x, T _y) : x(_x), y(_y) { }
   Point() { }
};

template <typename T>
struct Rect;

template<typename T>
ostream & operator<<(ostream &os, const Rect<T> & m) {
	os << "hi";
    //os << "(" << m.corners[0].x << ", " << m.corners[0].y << "), "
    //   << "(" << m.corners[2].x << ", " << m.corners[2].y << "), "
    //   << "(" << m.corners[3].x << ", " << m.corners[3].y << "), "
    //   << "(" << m.corners[1].x << ", " << m.corners[1].y << ")";
    return os;
}


template <typename T>
struct Rect {
   Rect(T x1, T y1, T x2, T y2, T x3, T y3, T x4, T y4) :
	   corners( {
	   Point<T>(x1, y1),
			   Point<T>(x2, y2),
			   Point<T>(x3, y3),
			   Point<T>(x4, y4)  } )
   {

   }
   Point<T> corners[4];
	friend ostream & operator<< <> (std::ostream & os, const Rect<T> & m);
};


#endif /* __INC_STRUCTS_H */
