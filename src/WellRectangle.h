#ifndef WELLCOORDINATES_H_
#define WELLCOORDINATES_H_

/*
 * WellCoordinates.h
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "geometry.h"

#include <string>

using namespace std;

template<typename T> class WellRectangle {
public:
   WellRectangle(const string & label);

	virtual ~WellRectangle() {
	}

	const string & getLabel() const {
		return label;
	}

	Point<T> & getCorner(unsigned cornerId) const;

private:
	const string label;
	Rect<T> rect;
};

#endif /* WELLCOORDINATES_H_ */
