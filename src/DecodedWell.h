#ifndef DECODEDWELL_H_
#define DECODEDWELL_H_

/*
 * DecodedWell.h
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "WellRectangle.h"

#include <string>

using namespace std;

class DecodedWell {
public:
	DecodedWell(const WellRectangle<unsigned> & wellRectangle);
	virtual ~DecodedWell();

	const WellRectangle<unsigned> & getWellRectangle() const {
		return wellRectangle;
	}

	const string & getMessage() const {
		return message;
	}

	void setMessage(const char * message, int messageLength);

	const string & getLabel() {
		return wellRectangle.getLabel();
	}

	void setCorner(unsigned cornerId, unsigned x, unsigned y);

	const Rect<unsigned> & getDecodedRect() const {
		return decodedRect;
	}

private:
	const WellRectangle<unsigned> & wellRectangle;
	Rect<unsigned> decodedRect;
	string message;
};

#endif /* DECODEDWELL_H_ */
