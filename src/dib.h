#ifndef __INCLUDE_DIB_H
#define __INCLUDE_DIB_H

#ifdef __cplusplus
 extern "C" {
 #endif

#include <stdio.h>

typedef struct sDib Dib;

//Colour palette
typedef struct sRgbQuad {
   unsigned char rgbBlue;
   unsigned char rgbGreen;
   unsigned char rgbRed;
   unsigned char rgbReserved;
} RgbQuad;

Dib * dibAllocate();
void dibAllocatePixelBuffer(Dib * dib);
void dibCopyHeaders(Dib * src, Dib * dest);
void dibDestroy(Dib * dib) ;
void readDibHeader(FILE * fh, Dib * dib) ;
void writeDibHeader(Dib * dib, FILE * fh);
unsigned dibGetHeight(Dib * dib);
unsigned dibGetWidth(Dib * dib);
unsigned dibGetRowPadBytes(Dib * dib);
unsigned dibGetBitsPerPixel(Dib * dib);
unsigned char * dibGetPixelBuffer(Dib * dib);
void readDibPixels(FILE * fh, Dib * dib);
void writeDibPixels(Dib * dib, FILE * fh);
void getDibPixel(Dib * dib, unsigned row, unsigned col, RgbQuad * quad);
unsigned getDibPixelGrayscale(Dib * dib, unsigned row, unsigned col);
void setDibPixel(Dib * dib, unsigned row, unsigned col, RgbQuad * quad);
unsigned char * dibGetPixelsNoPadding(Dib * dib);
void dibSetPixelsNoPadding(Dib * dib, unsigned char * pixels);
void setDibPixelGrayscale(Dib * dib, unsigned row, unsigned col,
                          unsigned value);
void dibConvertGrayscale(Dib * src, Dib * dest);
void sobelEdgeDetection(Dib * src, Dib * dest);
void laplaceEdgeDetection(Dib * src, Dib * dest);
void histEqualization(Dib * src, Dib * dest);

 #ifdef __cplusplus
 }
 #endif 

#endif /* __INCLUDE_DIB_H */
