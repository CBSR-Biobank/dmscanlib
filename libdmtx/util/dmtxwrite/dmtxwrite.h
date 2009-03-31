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

/* $Id: dmtxwrite.h 314 2008-06-09 19:32:23Z mblaughton $ */

#ifndef __DMTXWRITE_H__
#define __DMTXWRITE_H__

#if ENABLE_NLS
# include <libintl.h>
# define _(String) gettext(String)
#else
# define _(String) String
#endif
#define N_(String) String

#define DMTXWRITE_BUFFER_SIZE 4096

typedef struct {
   DmtxRgb color;
   DmtxRgb bgColor;
   char format;
   char *inputPath;
   char *outputPath;
   char preview;
   int rotate;
   int sizeIdx;
   int verbose;
   int mosaic;
   int dpi;
} UserOptions;

static void InitUserOptions(UserOptions *options);
static int HandleArgs(UserOptions *options, int *argcp, char **argvp[], DmtxEncode *encode);
static void ReadData(UserOptions *options, int *codeBuffer, unsigned char *codeBufferSize);
static void ShowUsage(int status);
static void WriteImagePng(UserOptions *options, DmtxEncode *encode);
static void WriteImagePnm(UserOptions *options, DmtxEncode *encode);
static void WriteAsciiBarcode(DmtxEncode *encode);
static void WriteCodewords(DmtxEncode *encode);

#endif
