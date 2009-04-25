#ifndef __INC_ImageProcessor_h
#define __INC_ImageProcessor_h

#include "dib.h"
#include "dmtx.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void decodeDib(char * filename);
DmtxImage* createDmtxImageFromFile(char* filename, Dib dib);
void decodeDmtxImage(DmtxImage* image);
DmtxImage* rotateImage(DmtxImage* src);
#endif /* __INC_ImageProcessor_h */
