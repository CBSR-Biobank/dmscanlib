#ifndef __INCLUDE_DIB_H
#define __INCLUDE_DIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

   typedef struct sDib * Dib;

   // File information header
   // provides general information about the file
   typedef struct  {
      unsigned short type;
      unsigned       size;
      unsigned short reserved1;
      unsigned short reserved2;
      unsigned       offset;
   } BitmapFileHeader;

   // Bitmap information header
   // provides information specific to the image data
   typedef struct {
      unsigned       size;
      unsigned       width;
      unsigned       height;
      unsigned short planes;
      unsigned short bitCount;
      unsigned       compression;
      unsigned       imageSize;
      unsigned       hPixelsPerMeter;
      unsigned       vPixelsPerMeter;
      unsigned       numColors;
      unsigned       numColorsImp;
   } BitmapInfoHeader;

   // Colour palette
   typedef struct sRgbQuad {
      unsigned char rgbBlue;
      unsigned char rgbGreen;
      unsigned char rgbRed;
      unsigned char rgbReserved;
   } RgbQuad;

   Dib dibAllocate();
   void dibAllocatePixelBuffer(Dib dib);
   void dibCopyHeaders(Dib src, Dib dest);
   void dibDestroy(Dib dib) ;
   void readDibHeader(FILE * fh, Dib dib) ;
   void writeDibHeader(Dib dib, FILE * fh);
   unsigned dibGetHeight(Dib dib);
   unsigned dibGetWidth(Dib dib);
   unsigned dibGetRowPadBytes(Dib dib);
   unsigned dibGetBitsPerPixel(Dib dib);
   unsigned char * dibGetPixelBuffer(Dib dib);
   void readDibPixels(FILE * fh, Dib dib);
   void writeDibPixels(Dib dib, FILE * fh);
   void getDibPixel(Dib dib, unsigned row, unsigned col, RgbQuad * quad);
   unsigned getDibPixelGrayscale(Dib dib, unsigned row, unsigned col);
   void setDibPixel(Dib dib, unsigned row, unsigned col, RgbQuad * quad);
   unsigned char * dibGetPixelsNoPadding(Dib dib);
   void dibSetPixelsNoPadding(Dib dib, unsigned char * pixels);
   void setDibPixelGrayscale(Dib dib, unsigned row, unsigned col,
                             unsigned value);
   void dibConvertGrayscale(Dib src, Dib dest);
   void sobelEdgeDetectionWithMask(Dib src, Dib dest, int mask1[3][3], int mask2[3][3]);
   void sobelEdgeDetection(Dib src, Dib dest);
   void laplaceEdgeDetection(Dib src, Dib dest);
   void histEqualization(Dib src, Dib dest);

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_DIB_H */
