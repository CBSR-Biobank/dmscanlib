/*
 * DecodedWell.cpp
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "DecodedWell.h"

DecodedWell::DecodedWell(WellRectangle<unsigned> & _wellCoordinates)
      :wellCoordinates(_wellCoordinates) {

}

DecodedWell::~DecodedWell() {
}

const WellRectangle<unsigned> & DecodedWell::getWellCoordinates() const {
	return wellCoordinates;
}

const string & DecodedWell::getMessage() const {
	return message;
}

void DecodedWell::setMessage(const char * message, int messageLength) {
   this->message.assign(message, messageLength);
}
