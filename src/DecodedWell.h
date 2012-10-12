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
	DecodedWell(WellRectangle<unsigned> & wellCoordinates);
	virtual ~DecodedWell();

	const WellRectangle<unsigned> & getWellCoordinates() const;

	const string & getMessage() const;

	void setMessage(const char * message, int messageLength);

	const string & getLabel() { return wellCoordinates.getLabel(); }

	void setCorner(unsigned cornerId, unsigned x, unsigned y);

private:
	const WellRectangle<unsigned> & wellCoordinates;
	Rect<unsigned> decodeRect;
	string message;
};

#endif /* DECODEDWELL_H_ */
