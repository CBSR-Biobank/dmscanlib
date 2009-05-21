#ifndef __INC_ImageProcessor_h
#define __INC_ImageProcessor_h

#include "dmtx.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

class Dib;

int decodeDib(char * filename, char* barcodes, int bufferSize, int* barcodeLength);
DmtxImage* createDmtxImageFromDib(Dib * dib);
int decodeSingleBarcode(DmtxImage* image, char* barcodes, int bufferSize, int* barcodeLength);
DmtxImage* rotateImage(DmtxImage* src);

#endif /* __INC_ImageProcessor_h */
