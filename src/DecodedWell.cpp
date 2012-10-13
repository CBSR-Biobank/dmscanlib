/*
 * DecodedWell.cpp
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "DecodedWell.h"

#include <glog/logging.h>

DecodedWell::DecodedWell(const WellRectangle<unsigned> & _wellRectangle)
      : wellRectangle(_wellRectangle), decodedRect(0, 0, 0, 0, 0, 0, 0, 0)
{
}

DecodedWell::~DecodedWell() {
}

void DecodedWell::setMessage(const char * message, int messageLength) {
   this->message.assign(message, messageLength);
}

void DecodedWell::setCorner(unsigned cornerId, unsigned x, unsigned y) {
     CHECK(cornerId < 4);
     decodedRect.corners[cornerId].x = x;
     decodedRect.corners[cornerId].y = y;
}
