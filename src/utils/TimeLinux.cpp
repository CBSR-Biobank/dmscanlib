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
#include <iostream>

#include <sys/time.h>

namespace dmscanlib {

namespace util {

void Time::getTime(slTime & tm) {
	gettimeofday(&tm, NULL);
}

void Time::getTimestamp(std::string & str_r) {
    char buf_a[100];

    // Fetch the current time
    char time_a[100];
    struct timeval thistime;

    gettimeofday(&thistime, NULL);
    strftime(time_a, sizeof(time_a), "%X", localtime(&thistime.tv_sec));
    snprintf(buf_a, sizeof (buf_a), "%s:%03ld ", time_a,
             thistime.tv_usec / 1000);

    str_r = buf_a;
}

void Time::difftime(slTime & start, slTime & end, slTime & diff) {
	diff.tv_sec = end.tv_sec - start.tv_sec;
	diff.tv_usec = end.tv_usec - start.tv_usec;
	if (diff.tv_usec < 0) {
		diff.tv_usec += 1000000;
	}
}

} /* namespace */

} /* namespace */

std::ostream & operator<<(std::ostream &os, dmscanlib::util::slTime & tm) {
	os << "sec/" << tm.tv_sec << " msec/" << tm.tv_usec/1000;
	return os;
}
