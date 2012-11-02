/*
 * WellCoordinates.cpp
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "WellRectangle.h"

#include <glog/logging.h>
#include <stdexcept>

namespace dmscanlib {

template<typename T>
WellRectangle<T>::WellRectangle(const char * _label,
		Point<T> & pt1, Point<T> & pt2, Point<T> & pt3, Point<T> & pt4) :
		label(_label), rect(pt1, pt2, pt3, pt4)
{
}

template<typename T>
WellRectangle<T>::WellRectangle(const char * _label, const Rect<T> & _rect) :
	label(_label), rect(_rect)
{
}

template<typename T>
WellRectangle<T>::WellRectangle(const char * _label, BoundingBox<T> & bbox)  :
	label(_label), rect(bbox)
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

} /* namespace */
