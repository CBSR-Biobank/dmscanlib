#ifndef WELLCOORDINATES_H_
#define WELLCOORDINATES_H_

/*
 * WellCoordinates.h
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "geometry.h"

#include <ostream>
#include <string>

namespace dmscanlib {

template<typename T>
class WellRectangle;

template<typename T>
std::ostream & operator<<(std::ostream &os, const WellRectangle<T> & m) {
	os << m.getLabel() << " - " << m.getRectangle();
	return os;
}

template<typename T>
class WellRectangle {
public:
	WellRectangle(const char * label, T x1, T y1, T x2, T y2,	T x3, T y3,
			T x4, T y4);

	WellRectangle(const char * label, const Rect<T> & rect);

	// bounding box constructor
	WellRectangle(const char * label, T x1, T y1, T x2, T y2);

	virtual ~WellRectangle() {
	}

	const std::string & getLabel() const {
		return label;
	}

	const Rect<T> & getRectangle() const {
		return rect;
	}

	const Point<T> & getCorner(unsigned cornerId) const;

	const T getCornerX(unsigned cornerId) const;

	const T getCornerY(unsigned cornerId) const;

private:
	const std::string label;
	const Rect<T> rect;

	friend std::ostream & operator<< <> (std::ostream & os, const WellRectangle<T> & m);
};

} /* namespace */

#endif /* WELLCOORDINATES_H_ */
