/*
 * BinRegion.cpp
 *
 *  Created on: 5-Jun-2009
 *      Author: loyola
 */

#include "BinRegion.h"

#include <limits>

BinRegion::BinRegion(unsigned o, unsigned min, unsigned max) :
	orientation(o), minimum(min), maximum(max), center(
			numeric_limits<unsigned>::max()), rank(
			numeric_limits<unsigned>::max()) {
}

BinRegion::~BinRegion() {
}

unsigned BinRegion::getCenter() {
	if (center == numeric_limits<unsigned>::max()) {
		center = (minimum + maximum) / 2;
	}
	return center;
}

void BinRegion::update(unsigned min, unsigned max) {
	minimum = min;
	maximum = max;
}

ostream & operator<<(ostream & os, BinRegion & r) {
	os << "min/" << r.minimum << " max/" << r.maximum << " rank/" << r.rank;
	return os;
}

bool BinRegionSort::operator()(BinRegion* const & a, BinRegion* const & b) {
	return (a->minimum < b->minimum);
}
