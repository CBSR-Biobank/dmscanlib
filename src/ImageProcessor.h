#ifndef __INC_ImageProcessor_h
#define __INC_ImageProcessor_h

#include "dib.h"
#include "dmtx.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void decodeDib(char * filename, char* barcodes, int bufferSize);
DmtxImage* createDmtxImageFromFile(char* filename, Dib dib);
void decodeDmtxImage(DmtxImage* image, char* barcodes, int bufferSize);
DmtxImage* rotateImage(DmtxImage* src);

#endif /* __INC_ImageProcessor_h */
