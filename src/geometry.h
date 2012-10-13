#ifndef __INC_STRUCTS_H
#define __INC_STRUCTS_H

template <typename T>
struct Point {
   T x;
   T y;

   Point (T _x, T _y) : x(_x), y(_y) { }
   Point() { }
};

template <typename T>
struct Rect {
   Point<T> corners[4];

   Rect(T x1, T y1, T x2, T y2, T x3, T y3, T x4, T y4) :
	   corners( {
	   Point<T>(x1, y1),
			   Point<T>(x2, y2),
			   Point<T>(x3, y3),
			   Point<T>(x4, y4)  } )
   {

   }
};


#endif /* __INC_STRUCTS_H */
