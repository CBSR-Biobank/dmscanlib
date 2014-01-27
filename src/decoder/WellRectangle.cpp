/*
 * WellCoordinates.cpp
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "WellRectangle.h"

#include <glog/logging.h>
#include <stdexcept>

namespace dmscanlib {

WellRectangle::WellRectangle(const char * _label, unsigned x, unsigned y, unsigned width, unsigned height) :
        label(_label), rect(x, y, width, height)
{
}

std::ostream & operator<<(std::ostream &os, const WellRectangle & m) {
    os << m.label << " - " << m.rect;
    return os;
}

} /* namespace */
