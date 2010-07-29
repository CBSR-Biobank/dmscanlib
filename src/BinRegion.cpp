/*
Dmscanlib is a software library and standalone application that scans 
and decodes libdmtx compatible test-tubes. It is currently designed 
to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
Copyright (C) 2010 Canadian Biosample Repository

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
 * BinRegion.cpp
 *
 *  Created on: 5-Jun-2009
 *      Author: loyola
 */

#include "BinRegion.h"

#include <limits>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif


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
