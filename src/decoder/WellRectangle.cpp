/*
 * WellCoordinates.cpp
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "WellRectangle.h"

#include <glog/logging.h>

template<typename T>
WellRectangle<T>::WellRectangle(const char * _label, T x1, T y1,
		T x2, T y2,	T x3, T y3, T x4, T y4) :
		label(_label), rect(x1, y1, x2, y2, x3, y3, x4, y4)
{
}

template<typename T>
WellRectangle<T>::WellRectangle(const char * _label, const Rect<T> & _rect) :
	label(_label), rect(_rect)
{
}

template<typename T>
WellRectangle<T>::WellRectangle(const char * _label, T x1, T y1, T x2, T y2)  :
	label(_label), rect(x1, y1, x1, y2, x2, y1, x2, y2)
{
}


template<typename T>
const T WellRectangle<T>::getCornerX(unsigned cornerId) const {
    CHECK(cornerId < 4);
    return rect.corners[cornerId].x;
}

template<typename T>
const T WellRectangle<T>::getCornerY(unsigned cornerId) const {
    CHECK(cornerId < 4);
    return rect.corners[cornerId].y;

}

template<typename T>
const Point<T> & WellRectangle<T>::getCorner(unsigned cornerId) const {
	CHECK(cornerId < 4);
	return rect.corners[cornerId];
}

template class WellRectangle<unsigned>;
template class WellRectangle<double>;
