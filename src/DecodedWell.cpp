/*
 * DecodedWell.cpp
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "DecodedWell.h"

DecodedWell::DecodedWell(WellRectangle<unsigned> & _wellRectangle)
      :wellRectangle(_wellRectangle) {
}

DecodedWell::~DecodedWell() {
}

void DecodedWell::setMessage(const char * message, int messageLength) {
   this->message.assign(message, messageLength);
}
