/*
Dmscanlib is a software library and standalone application that scans 
and decodes libdmtx compatible test-tubes. It is currently designed 
to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
Copyright (C) 2010 Canadian Biosample Repository

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Time.h"

#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>

#include <sys/time.h>

namespace dmscanlib {

namespace util {

Time::Time() {
	gettimeofday(&time, NULL);
}

Time::Time(Time & that) {
	time = that.time;
}

std::unique_ptr<Time> Time::difftime(const Time & that) {
	std::unique_ptr<Time> result(new Time(*this));

	result->time.tv_sec = time.tv_sec - that.time.tv_sec;
	result->time.tv_usec = time.tv_usec - that.time.tv_usec;
	if (result->time.tv_usec < 0) {
		result->time.tv_usec += 1000000;
	}

	return result;
}

std::ostream & operator<<(std::ostream &os, const dmscanlib::util::Time & tm) {
	os << std::setw(2) << tm.time.tv_sec << "." << std::setw(3) << tm.time.tv_usec/1000;
	return os;
}

} /* namespace */

} /* namespace */
