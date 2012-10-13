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
		T x2, T y2,	T x3, T y3, T x4, T y4) : label(_label) {
	rect.corners[0].x = x1;
	rect.corners[0].y = y1;

	rect.corners[1].x = x2;
	rect.corners[1].y = y2;

	rect.corners[2].x = x3;
	rect.corners[2].y = y3;

	rect.corners[3].x = x4;
	rect.corners[3].y = y4;
}

template<typename T>
Point<T> & WellRectangle<T>::getCorner(unsigned cornerId) const {
	CHECK(cornerId < 4);
}
