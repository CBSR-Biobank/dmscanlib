/*
 * WellCoordinates.cpp
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "WellRectangle.h"

#include <glog/logging.h>

template<typename T>
WellRectangle<T>::WellRectangle(const string & _label, T left, T top,
		T right, T bottom) : label(_label) {
	this->rect.corners[0].x = left;
	this->rect.corners[0].y = top;

	this->rect.corners[1].x = right;
	this->rect.corners[1].y = top;

	this->rect.corners[2].x = left;
	this->rect.corners[2].y = bottom;

	this->rect.corners[3].x = right;
	this->rect.corners[3].y = bottom;
}

template<typename T>
Point<T> & WellRectangle<T>::getCorner(unsigned cornerId) const {
	CHECK(cornerId < 4);
}
