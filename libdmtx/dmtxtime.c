/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2008, 2009 Mike Laughton

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Contact: mike@dragonflylogic.com
*/

/* $Id: dmtxtime.c 759 2009-02-26 16:48:59Z mblaughton $ */

/**
 * @file dmtxtime.c
 * @brief Time handling
 */

#define DMTX_USEC_PER_SEC 1000000

#if defined(HAVE_SYS_TIME_H) && defined(HAVE_GETTIMEOFDAY)

#include <sys/time.h>
#include <time.h>
#define DMTX_TIME_PREC_USEC 1

/**
 * @brief  GETTIMEOFDAY version
 * @return Time now
 */
extern DmtxTime
dmtxTimeNow(void)
{
   DmtxPassFail err;
   struct timeval tv;
   DmtxTime tNow;

   err = gettimeofday(&tv, NULL);
   if(err != 0)
      ; /* XXX handle error better here */

   tNow.sec = tv.tv_sec;
   tNow.usec = tv.tv_usec;

   return tNow;
}

#elif defined(_MSC_VER)

#include <Windows.h>
#define DMTX_TIME_PREC_USEC 1

/**
 * @brief  MICROSOFT VC++ version
 * @return Time now
 */
extern DmtxTime
dmtxTimeNow(void)
{
   FILETIME ft;
   unsigned __int64 tm;
   DmtxTime tNow;

   GetSystemTimeAsFileTime(&ft);

   tm = ft.dwHighDateTime;
   tm <<= 32;
   tm |= ft.dwLowDateTime;
   tm /= 10;

   tNow.sec = tm / 1000000UL;
   tNow.usec = tm % 1000000UL;

   return tNow;
}

#else

#include <time.h>
#define DMTX_TIME_PREC_USEC 1000000

/**
 * @brief  Generic 1 second resolution version
 * @return Time now
 */
extern DmtxTime
dmtxTimeNow(void)
{
   time_t s;
   DmtxTime tNow;

   s = time(NULL);
   if(errno != 0)
      ; /* XXX handle error better here */

   tNow.sec = s;
   tNow.usec = 0;

   return tNow;
}

#endif

/**
 * @brief  Add milliseconds to time t
 * @param  t
 * @param  msec
 * @return Adjusted time
 */
extern DmtxTime
dmtxTimeAdd(DmtxTime t, long msec)
{
   int usec;

   usec = msec * 1000;

   /* Ensure that time difference will register on local system */
   if(usec > 0 && usec < DMTX_TIME_PREC_USEC)
      usec = DMTX_TIME_PREC_USEC;

   /* Add time */
   t.sec += usec/DMTX_USEC_PER_SEC;
   t.usec += usec%DMTX_USEC_PER_SEC;

   /* Roll extra usecs into secs */
   while(t.usec >= DMTX_USEC_PER_SEC) {
      t.sec++;
      t.usec -= DMTX_USEC_PER_SEC;
   }

   return t;
}

/**
 * @brief  Determine whether the received timeout has been exceeded
 * @param  timeout
 * @return 1 (true) | 0 (false)
 */
extern int
dmtxTimeExceeded(DmtxTime timeout)
{
   DmtxTime now;

   now = dmtxTimeNow();

   return (now.sec > timeout.sec || (now.sec == timeout.sec && now.usec > timeout.usec));
}

#undef DMTX_TIME_PREC_USEC
#undef DMTX_USEC_PER_SEC
