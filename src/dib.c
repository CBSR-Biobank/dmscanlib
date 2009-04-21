/*******************************************************************************
 * Device Independent Bitmap
 ******************************************************************************/

#include "dib.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifndef _VISUALC_
#include <strings.h>
#endif

//#define _UNIT_TEST_

struct sDib {
   BitmapFileHeader fileHeader;
   BitmapInfoHeader infoHeader;
   unsigned bytesPerPixel;
   unsigned rowPaddingBytes;
   unsigned char * pixels;
};

Dib dibAllocate() {
   return (Dib) malloc(sizeof(Dib));
}

void dibCopyHeaders(Dib src, Dib dest) {
   dest->fileHeader = src->fileHeader;
   dest->infoHeader = src->infoHeader;
}

void dibAllocatePixelBuffer(Dib dib) {
   dib->pixels = (unsigned char *) malloc(dib->infoHeader.imageSize);
}

void dibDestroy(Dib dib) {
   free(dib->pixels);
   free(dib);
}

/**
 * All values in little-endian except for BitmapFileHeader.type.
 */
void readDibHeader(FILE * fh, Dib dib) {
   unsigned char fileHeaderRaw[0xE];
   unsigned char infoHeaderRaw[0x28];

   memset(&dib->fileHeader, 0, sizeof(dib->fileHeader));
   memset(&dib->infoHeader, 0, sizeof(dib->infoHeader));

   fread(fileHeaderRaw, 1, sizeof(fileHeaderRaw), fh);
   fread(infoHeaderRaw, 1, sizeof(infoHeaderRaw), fh);

   dib->fileHeader.type      = *(unsigned short *)&fileHeaderRaw[0];
   dib->fileHeader.size      = *(unsigned *)&fileHeaderRaw[2];
   dib->fileHeader.reserved1 = *(unsigned short *)&fileHeaderRaw[6];
   dib->fileHeader.reserved2 = *(unsigned short *)&fileHeaderRaw[8];
   dib->fileHeader.offset    = *(unsigned *)&fileHeaderRaw[0xA];

   dib->infoHeader.size            = *(unsigned *)&infoHeaderRaw[0];
   dib->infoHeader.width           = *(unsigned *)&infoHeaderRaw[0x12 - 0xE];
   dib->infoHeader.height          = *(unsigned *)&infoHeaderRaw[0x16 - 0xE];
   dib->infoHeader.planes          = *(unsigned short *)&infoHeaderRaw[0x1A - 0xE];
   dib->infoHeader.bitCount        = *(unsigned short *)&infoHeaderRaw[0x1C - 0xE];
   dib->infoHeader.compression     = *(unsigned *)&infoHeaderRaw[0x1E - 0xE];
   dib->infoHeader.imageSize       = *(unsigned *)&infoHeaderRaw[0x22 - 0xE];
   dib->infoHeader.hPixelsPerMeter = *(unsigned *)&infoHeaderRaw[0x26 - 0xE];
   dib->infoHeader.vPixelsPerMeter = *(unsigned *)&infoHeaderRaw[0x2A - 0xE];
   dib->infoHeader.numColors       = *(unsigned *)&infoHeaderRaw[0x2E - 0xE];
   dib->infoHeader.numColorsImp    = *(unsigned *)&infoHeaderRaw[0x32 - 0xE];

   dib->bytesPerPixel = dib->infoHeader.bitCount >> 3;
   dib->rowPaddingBytes = (dib->infoHeader.width * dib->bytesPerPixel) & 0x3;
}

void writeDibHeader(Dib dib, FILE * fh) {
   unsigned char fileHeaderRaw[0xE];
   unsigned char infoHeaderRaw[0x28];

   *(unsigned short *)&fileHeaderRaw[0] = dib->fileHeader.type;
   *(unsigned *)&fileHeaderRaw[2]       = dib->fileHeader.size;
   *(unsigned short *)&fileHeaderRaw[6] = dib->fileHeader.reserved1;
   *(unsigned short *)&fileHeaderRaw[8] = dib->fileHeader.reserved2;
   *(unsigned *)&fileHeaderRaw[0xA]     = dib->fileHeader.offset;

   *(unsigned *)&infoHeaderRaw[0]                = dib->infoHeader.size;
   *(unsigned *)&infoHeaderRaw[0x12 - 0xE]       = dib->infoHeader.width;
   *(unsigned *)&infoHeaderRaw[0x16 - 0xE]       = dib->infoHeader.height;
   *(unsigned short *)&infoHeaderRaw[0x1A - 0xE] = dib->infoHeader.planes;
   *(unsigned short *)&infoHeaderRaw[0x1C - 0xE] = dib->infoHeader.bitCount;
   *(unsigned *)&infoHeaderRaw[0x1E - 0xE]       = dib->infoHeader.compression;
   *(unsigned *)&infoHeaderRaw[0x22 - 0xE]       = dib->infoHeader.imageSize;
   *(unsigned *)&infoHeaderRaw[0x26 - 0xE]       = dib->infoHeader.hPixelsPerMeter;
   *(unsigned *)&infoHeaderRaw[0x2A - 0xE]       = dib->infoHeader.vPixelsPerMeter;
   *(unsigned *)&infoHeaderRaw[0x2E - 0xE]       = dib->infoHeader.numColors;
   *(unsigned *)&infoHeaderRaw[0x32 - 0xE]       = dib->infoHeader.numColorsImp;

   fwrite(fileHeaderRaw, 1, sizeof(fileHeaderRaw), fh);
   fwrite(infoHeaderRaw, 1, sizeof(infoHeaderRaw), fh);
}

unsigned dibGetHeight(Dib dib) {
   return dib->infoHeader.height;
}

unsigned dibGetWidth(Dib dib) {
   return dib->infoHeader.width;
}

unsigned dibGetRowPadBytes(Dib dib) {
   return dib->rowPaddingBytes;
}

unsigned dibGetBitsPerPixel(Dib dib) {
   return dib->infoHeader.bitCount;
}

unsigned char * dibGetPixelBuffer(Dib dib) {
   return dib->pixels;
}

void readDibPixels(FILE * fh, Dib dib) {
   assert(dib->fileHeader.type == 0x4d42);
   assert(dib->infoHeader.imageSize != 0);
   dib->pixels = (unsigned char *) malloc(dib->infoHeader.imageSize);
   fread(dib->pixels, 1, dib->infoHeader.imageSize, fh);
}

void writeDibPixels(Dib dib, FILE * fh) {
   assert(dib->fileHeader.type == 0x4d42);
   assert(dib->infoHeader.imageSize != 0);
   fwrite(dib->pixels, 1, dib->infoHeader.imageSize, fh);
}

void getDibPixel(Dib dib, unsigned row, unsigned col, RgbQuad * quad) {
   unsigned rowBytes = dib->infoHeader.width * dib->bytesPerPixel + dib->rowPaddingBytes;
   unsigned char * ptr = (dib->pixels + row * rowBytes + col * dib->bytesPerPixel);

   assert(dib->infoHeader.bitCount == 24);
   quad->rgbRed      = ptr[0];
   quad->rgbGreen    = ptr[1];
   quad->rgbBlue     = ptr[2];
   quad->rgbReserved = 0;
}

unsigned getDibPixelGrayscale(Dib dib, unsigned row, unsigned col) {
   unsigned rowBytes = dib->infoHeader.width * dib->bytesPerPixel + dib->rowPaddingBytes;
   unsigned char * ptr = (dib->pixels + row * rowBytes + col * dib->bytesPerPixel);

   if (dib->infoHeader.bitCount == 24) {
      ptr = (dib->pixels + row * rowBytes + col * dib->bytesPerPixel);
      return (unsigned) (0.3 * ptr[0] + 0.59 * ptr[1] + 0.11 * ptr[2]);
   }
   else if (dib->infoHeader.bitCount == 8) {
      return (unsigned) *ptr;
   }
   assert(0); // bitCount not implemented yet
   return  0;
}

void setDibPixel(Dib dib, unsigned row, unsigned col, RgbQuad * quad) {
   unsigned padding = (dib->infoHeader.width * dib->bytesPerPixel) & 0x3;
   unsigned rowBytes = dib->infoHeader.width * dib->bytesPerPixel + padding;
   unsigned char * ptr = (dib->pixels + row * rowBytes + col * dib->bytesPerPixel);

   if (dib->infoHeader.bitCount == 24) {
      ptr[0] = quad->rgbRed;
      ptr[1] = quad->rgbGreen;
      ptr[2] = quad->rgbBlue;
   }
   else {
      assert(0); // can't assign RgbQuad to dib
   }
}

void setDibPixelGrayscale(Dib dib, unsigned row, unsigned col,
                          unsigned value) {
   unsigned padding = (dib->infoHeader.width * dib->bytesPerPixel) & 0x3;
   unsigned rowBytes = dib->infoHeader.width * dib->bytesPerPixel + padding;
   unsigned char * ptr = (dib->pixels + row * rowBytes + col * dib->bytesPerPixel);

   if (dib->infoHeader.bitCount == 24) {
      ptr[0] = value;
      ptr[1] = value;
      ptr[2] = value;
   }
   else if (dib->infoHeader.bitCount == 8) {
      *ptr = value;
   }
   else {
      assert(0); // can't assign RgbQuad to dib
   }
}

void dibConvertGrayscale(Dib src, Dib dest) {
   unsigned width = src->infoHeader.width;
   unsigned height = src->infoHeader.height;
   unsigned row, col;

   dest->fileHeader = src->fileHeader;
   dest->infoHeader = src->infoHeader;
   dibAllocatePixelBuffer(dest);

   for (row = 0; row < height; ++row) {
      for (col = 0; col < width; ++col) {
         setDibPixelGrayscale(dest, row, col,
                              getDibPixelGrayscale(src, row, col));
      }
   }
}

void sobelEdgeDetectionWithMask(Dib src, Dib dest, int mask1[3][3], int mask2[3][3]) {
   int I, J;
   unsigned Y, X, SUM;
   long sum1, sum2;
   unsigned width = src->infoHeader.width;
   unsigned height = src->infoHeader.height;

   assert(mask1 != NULL);

   for (Y = 0; Y <= height - 1; ++Y)  {
      for (X = 0; X <= width - 1 ; ++X)  {
         sum1 = 0;
         sum2 = 0;

         /* image boundaries */
         if ((Y == 0) || (Y == height - 1)
             || (X == 0) || (X == width - 1)) {
            SUM = 0;
         }
         else {
            /* Convolution starts here */

            for (I = -1; I <= 1; ++I)  {
               for (J = -1; J <= 1; ++J)  {
                  sum1 += getDibPixelGrayscale(src, Y + J, X + I)
                     * mask1[I+1][J+1];
               }
            }

            if (mask2 != NULL) {
               for (I = -1; I <= 1; ++I)  {
                  for (J = -1; J <= 1; ++J)  {
                     sum2 += getDibPixelGrayscale(src, Y + J, X + I)
                        * mask2[I+1][J+1];
                  }
               }
            }

            /*---GRADIENT MAGNITUDE APPROXIMATION (Myler p.218)----*/
            SUM = abs(sum1) + abs(sum2);
         }

         if (SUM > 255) SUM = 255;
         if (SUM < 0) SUM = 0;

         setDibPixelGrayscale(dest, Y, X, 255 - SUM);
      }
   }
}

/**
 * Taken from: http://www.pages.drexel.edu/~weg22/edge.html
 */
void sobelEdgeDetection(Dib src, Dib dest) {
   int GX[3][3];
   int GY[3][3];

   dest->fileHeader = src->fileHeader;
   dest->infoHeader = src->infoHeader;
   dibAllocatePixelBuffer(dest);

   /* 3x3 GX Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html */
   GX[0][0] = -1; GX[0][1] = 0; GX[0][2] = 1;
   GX[1][0] = -2; GX[1][1] = 0; GX[1][2] = 2;
   GX[2][0] = -1; GX[2][1] = 0; GX[2][2] = 1;

   /* 3x3 GY Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html */
   GY[0][0] =  1; GY[0][1] =  2; GY[0][2] =  1;
   GY[1][0] =  0; GY[1][1] =  0; GY[1][2] =  0;
   GY[2][0] = -1; GY[2][1] = -2; GY[2][2] = -1;

   sobelEdgeDetectionWithMask(src, dest, GX, GY);
}

/**
 * Taken from: http://www.pages.drexel.edu/~weg22/edge.html
 */
void laplaceEdgeDetection(Dib src, Dib dest) {
   unsigned width = src->infoHeader.width;
   unsigned height = src->infoHeader.height;
   unsigned Y, X, SUM;
   int I, J, MASK[5][5];

   dest->fileHeader = src->fileHeader;
   dest->infoHeader = src->infoHeader;
   dibAllocatePixelBuffer(dest);

   /* 5x5 Laplace mask.  Ref: Myler Handbook p. 135 */
   MASK[0][0] = -1; MASK[0][1] = -1; MASK[0][2] = -1; MASK[0][3] = -1; MASK[0][4] = -1;
   MASK[1][0] = -1; MASK[1][1] = -1; MASK[1][2] = -1; MASK[1][3] = -1; MASK[1][4] = -1;
   MASK[2][0] = -1; MASK[2][1] = -1; MASK[2][2] = 24; MASK[2][3] = -1; MASK[2][4] = -1;
   MASK[3][0] = -1; MASK[3][1] = -1; MASK[3][2] = -1; MASK[3][3] = -1; MASK[3][4] = -1;
   MASK[4][0] = -1; MASK[4][1] = -1; MASK[4][2] = -1; MASK[4][3] = -1; MASK[4][4] = -1;

   for (Y = 0; Y <= height - 1; ++Y)  {
      for (X = 0; X <= width - 1; ++X)  {
         SUM = 0;

         /* image boundaries */
         if ((Y <= 1) || (Y >= height - 2) || (X <= 1) || (X >= width - 2)) {
            SUM = 0;
         }
         else {
            /* Convolution starts here */
            for (I = -2; I <= 2; ++I)  {
               for (J = -2; J <= 2; ++J)  {
                  SUM = SUM + getDibPixelGrayscale(src, Y + J, X + I)
                     * MASK[I+2][J+2];
               }
            }
         }

         if (SUM > 255)  SUM = 255;
         if (SUM < 0)    SUM = 0;

         setDibPixelGrayscale(dest, Y, X, 255 - SUM);
      }
   }
}

void histEqualization(Dib src, Dib dest) {
   unsigned width = src->infoHeader.width;
   unsigned height = src->infoHeader.height;
   unsigned imgSize = width * height;
   unsigned histogram[256];
   double sum[256], runningSum;
   unsigned row, col, i;

   dest->fileHeader = src->fileHeader;
   dest->infoHeader = src->infoHeader;
   dibAllocatePixelBuffer(dest);

   memset(&histogram, 0, sizeof(histogram));
   memset(&sum, 0, sizeof(sum));

   for (row = 0; row < height; ++row) {
      for (col = 0; col < width; ++col) {
         histogram[getDibPixelGrayscale(src, row, col)]++;
      }
   }

   for (i = 0, runningSum = 0; i <= 255; ++i) {
      runningSum += (histogram[i] / (double) imgSize);
      sum[i] = runningSum;
   }

   for (row = 0; row < height; ++row) {
      for (col = 0; col < width; ++col) {
         setDibPixelGrayscale(
            dest, row, col,
            256 * (int) sum[getDibPixelGrayscale(src, row, col)]);
      }
   }
}

#ifdef _UNIT_TEST_

#include <getopt.h>

/* Allowed command line arguments.  */
static struct option long_options[] = {
   { "test",    no_argument, NULL, 't' },
   { 0, 0, 0, 0 }
};

void printUsage() {
   printf("Usage: \n");
}


int main(int argc, char ** argv) {
   int ch;

   while (1) {
      int option_index = 0;

      ch = getopt_long (argc, argv, "t", long_options, &option_index);

      if (ch == -1) break;
      switch (ch) {

         case 't':
            break;

         case '?':
         case 'h':
            printUsage(-1);
            exit(0);
            break;

         default:
            printf("?? getopt returned character code %c ??\n", ch);
      }
   }

   if (optind >= argc) {
      // not enough command line arguments, print usage message
      printUsage();
      exit(-1);
   }

   char * filename = argv[optind];
   printf("  opening file %s...\n", filename);
   FILE * fh = fopen(filename, "r");
   Dib dib;
   readDibHeader(fh, &dib);
   readDibPixels(fh, &dib);
   fclose(fh);

   // convert to a grayscale iamge
   Dib gsDib;

   //dibConvertGrayscale(&dib, &gsDib);
   //sobelEdgeDetection(&dib, &gsDib);
   //laplaceEdgeDetection(&dib, &gsDib);
   histEqualization(&dib, &gsDib);

   fh = fopen("out.bmp", "w");
   writeDibHeader(&gsDib, fh);
   writeDibPixels(&gsDib, fh);
   fclose(fh);
   dibDestroy(&dib);
   dibDestroy(&gsDib);
}

#endif /* _UNIT_TEST_ */                        \

