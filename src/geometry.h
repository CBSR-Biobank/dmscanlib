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
};


#endif /* __INC_STRUCTS_H */
