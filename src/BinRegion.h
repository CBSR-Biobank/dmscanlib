#ifndef BINREGION_H_
#define BINREGION_H_

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
