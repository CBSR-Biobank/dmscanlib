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

class BinRegion {
public:
	BinRegion(unsigned orientation, unsigned min, unsigned max);
	~BinRegion();

	void update(unsigned min, unsigned max);

	void setMin(unsigned min) {
		minimum = min;
	}

	unsigned getMin() {
		return minimum;
	}

	void setMax(unsigned max) {
		maximum = max;
	}

	unsigned getMax() {
		return maximum;
	}

	void setRank(unsigned r) {
		rank = r;
	}

	unsigned getRank() {
		return rank;
	}

	static const unsigned ORIENTATION_HOR = 0;
	static const unsigned ORIENTATION_VER = 1;

private:
	unsigned orientation;
	unsigned minimum;
	unsigned maximum;
	unsigned rank;

	friend ostream & operator<<(ostream & os, BinRegion & r);
	friend struct BinRegionSort;
};

struct BinRegionSort {
	bool operator()(BinRegion* const& a, BinRegion* const& b);
};

#endif /* BINREGION_H_ */
