/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (c) 2008 Mike Laughton

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

/* $Id: dmtxread.h 124 2008-04-13 01:38:03Z mblaughton $ */

#ifndef __DMTXUTIL_H__
#define __DMTXUTIL_H__

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

extern int StringToInt(int *numberInt, char *numberString, char **terminate);
extern void FatalError(int errorCode, char *fmt, ...);
extern char *Basename(char *path);

#endif
