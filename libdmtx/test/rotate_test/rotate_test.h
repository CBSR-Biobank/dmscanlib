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

/* $Id: rotate_test.h 641 2009-02-03 00:25:22Z mblaughton $ */

#ifndef __SCANDEMO_H__
#define __SCANDEMO_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "../../dmtx.h"
#include "image.h"
#include "display.h"
#include "callback.h"

#define max(X,Y) (X > Y) ? X : Y
#define min(X,Y) (X < Y) ? X : Y

extern GLfloat       view_rotx;
extern GLfloat       view_roty;
extern GLfloat       view_rotz;
extern GLfloat       angle;

extern GLuint        barcodeTexture;
extern GLint         barcodeList;

extern DmtxImage     *gImage;
extern unsigned char *capturePxl;
extern unsigned char *texturePxl;
extern unsigned char *passOnePxl;
extern unsigned char *passTwoPxl;

extern char *gFilename[];
extern int gFileIdx;
extern int gFileCount;

#endif
