/*
Scanlib is a software library and standalone application that scans 
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
/*
 * Util.cpp
 *
 *  Created on: Jun 4, 2009
 *      Author: nelson
 */

#include "Util.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#if defined (WIN32) && ! defined(__MINGW32__)
#include <time.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif

bool Util::strToNum(const char * str, int & number, unsigned base) {
	char  * end = 0;
	number = strtoul(str, &end, base);
	return (*end == 0);
}

bool Util::strToNum(string & str, int & number, unsigned base) {
	char  * end = 0;
	number = strtoul(str.c_str (), &end, base);
	return (*end == 0);
}

bool Util::strToNum(string & str, double & number) {
	char  * end = 0;
	number = strtod(str.c_str (), &end);
	return (*end == 0);

}

void Util::getTime(slTime & tm) {
#if defined (WIN32) && ! defined(__MINGW32__)
	time(&tm);
#else
	gettimeofday(&tm, NULL);
#endif
}

void Util::getTimestamp(std::string & str_r) {
    char buf_a[100];

#if defined (WIN32) && ! defined(__MINGW32__)
   time_t ltime;
   struct _timeb tstruct;
   char timebuf[26];
   errno_t err;

   time( &ltime );
   err = ctime_s(timebuf, 26, &ltime);
   if (err) {
      cerr << "ctime_s failed due to an invalid argument.";
      exit(1);
   }
   _ftime_s( &tstruct );
   sprintf_s(buf_a, "%.8s:%03u ", timebuf + 11, tstruct.millitm);
#else
    // Fetch the current time
    char time_a[100];
    struct timeval thistime;

    gettimeofday(&thistime, NULL);
    strftime(time_a, sizeof(time_a), "%X", localtime(&thistime.tv_sec));
    snprintf(buf_a, sizeof (buf_a), "%s:%03ld ", time_a,
             thistime.tv_usec / 1000);
#endif

    str_r = buf_a;
}

void Util::difftiime(slTime & start, slTime & end, slTime & diff) {
#if defined (WIN32) && ! defined(__MINGW32__)
	diff = end - start;
#else
	diff.tv_sec = end.tv_sec - start.tv_sec;
	diff.tv_usec = end.tv_usec - start.tv_usec;
	if (diff.tv_usec < 0) {
		diff.tv_usec += 1000000;
	}
#endif
}

#ifndef _VISUALC_
ostream & operator<<(ostream &os, slTime & tm) {
#if defined (WIN32) && ! defined(__MINGW32__)
#else
	os << "sec/" << tm.tv_sec << " msec/" << tm.tv_usec/1000;
#endif
	return os;
}
#endif
