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

#include "TimeUtil.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <time.h>
#include <sys/timeb.h>

void Util::getTime(slTime & tm) {
	time(&tm);
}

void Util::getTimestamp(std::string & str_r) {
    char buf_a[100];

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

    str_r = buf_a;
}

void Util::difftiime(slTime & start, slTime & end, slTime & diff) {
	diff = end - start;
}
