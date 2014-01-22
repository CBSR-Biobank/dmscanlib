/*
 * WellCoordinates.cpp
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "WellRectangle.h"

#include <glog/logging.h>
#include <stdexcept>

namespace dmscanlib {

WellRectangle<T>::WellRectangle(const char * _label, unsigned x, unsigned y, unsigned width, unsigned height) :
        label(_label), rect(x, y, width, height)
{
}

} /* namespace */
