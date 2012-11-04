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
	os << m.label << " - " << m.rect;
	return os;
}

template<typename T>
class WellRectangle {
public:
	WellRectangle(const char * label, const Rect<T> & rect);

	WellRectangle(const char * label, BoundingBox<T> & bbox);

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
