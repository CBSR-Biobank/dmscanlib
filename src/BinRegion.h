#ifndef BINREGION_H_
#define BINREGION_H_
/*
Scanlib is a software library and standalone application that scans 
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
 * BinRegion.h
 *
 *  Created on: 5-Jun-2009
 *      Author: loyola
 */

#include <ostream>

using namespace std;

/**
 * Represents a region in an image that contains either a row or column of
 * 2D barcode tubes.
 */
class BinRegion {
public:
	BinRegion(unsigned orientation, unsigned min, unsigned max);
	~BinRegion();

	void update(unsigned min, unsigned max);

	unsigned getMin() {
		return minimum;
	}

	void setMin(unsigned min) {
		minimum = min;
	}

	unsigned getMax() {
		return maximum;
	}

	void setMax(unsigned max) {
		maximum = max;
	}

	unsigned getCenter();

	unsigned getRank() {
		return rank;
	}

	void setRank(unsigned r) {
		rank = r;
	}

	unsigned getId() {
		return id;
	}

	void setId(unsigned i) {
		id = i;
	}

	static const unsigned ORIENTATION_HOR = 0;
	static const unsigned ORIENTATION_VER = 1;

private:
	unsigned orientation;
	unsigned minimum;
	unsigned maximum;
	unsigned center;
	unsigned rank;
	unsigned id;

	friend ostream & operator<<(ostream & os, BinRegion & r);
	friend struct BinRegionSort;
};

struct BinRegionSort {
	bool operator()(BinRegion* const& a, BinRegion* const& b);
};

#endif /* BINREGION_H_ */
