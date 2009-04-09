/*
libdmtx - Data Matrix Encoding/Decoding Library

Copyright (C) 2007, 2008, 2009 Mike Laughton

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

Contact: mblaughton@users.sourceforge.net
*/

/* $Id: image.h 598 2009-01-20 13:59:54Z mblaughton $ */

#ifndef __IMAGE_H__
#define __IMAGE_H__

#define IMAGE_NO_ERROR  0
#define IMAGE_ERROR     1
#define IMAGE_NOT_PNG   2

typedef enum {
   ColorWhite  = 0x01 << 0,
   ColorRed    = 0x01 << 1,
   ColorGreen  = 0x01 << 2,
   ColorBlue   = 0x01 << 3,
   ColorYellow = 0x01 << 4
} ColorEnum;

/*void captureImage(DmtxImage *img, DmtxImage *imgTmp);*/
unsigned char *loadTextureImage(int *width, int *height);
unsigned char *loadPng(char *filename, int *width, int *height);
void plotPoint(DmtxImage *img, float rowFloat, float colFloat, int targetColor);
int clampRGB(float color);

#endif
