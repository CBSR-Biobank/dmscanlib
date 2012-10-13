/*******************************************************************************
 *
 * Device Independent Bitmap
 *
 ******************************************************************************
 *
 * Dmscanlib is a software library and standalone application that scans
 * and decodes libdmtx compatible test-tubes. It is currently designed
 * to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 * Copyright (C) 2010 Canadian Biosample Repository
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifdef _VISUALC_
#pragma warning(disable : 4996)  // disable fopen warnings
#endif

#ifdef WIN32
#undef ERROR
#endif

#include "Dib.h"
#include "RgbQuad.h"
#include "DmScanLib.h"

#include <glog/logging.h>
#include <stdio.h>
#include <algorithm>
#include <math.h>

using namespace std;

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif


/* File information header
 * provides general information about the file
 */
struct BitmapFileHeader {
   unsigned short type;
   unsigned size;
   unsigned short reserved1;
   unsigned short reserved2;
   unsigned offset;
};

/* Bitmap information header
 * provides information specific to the image data
 */
struct BitmapInfoHeader {
   unsigned size;
   unsigned width;
   unsigned height;
   unsigned short planes;
   unsigned short bitCount;
   unsigned compression;
   unsigned imageSize;
   unsigned hPixelsPerMeter;
   unsigned vPixelsPerMeter;
   unsigned numColors;
   unsigned numColorsImp;
};

const double Dib::UNSHARP_RAD = 8.0;
const double Dib::UNSHARP_DEPTH = 1.1;
const unsigned Dib::GAUSS_WIDTH = 12;
const unsigned Dib::GAUSS_FACTORS[GAUSS_WIDTH] = {
   1, 11, 55, 165, 330, 462, 462, 330, 165, 55, 11, 1
};

const unsigned Dib::GAUSS_SUM = 2048;

// expects sharp image
const float Dib::BLUR_KERNEL[9] = {
   0.0f, 0.2f, 0.0f, 0.2f, 0.2f, 0.2f, 0.0f, 0.2f, 0.0f
};

// expects sharp image
const float Dib::BLANK_KERNEL[9] = {
   0.06185567f, 0.12371134f, 0.06185567f,
   0.12371134f, 0.257731959f, 0.12371134f,
   0.06185567f, 0.12371134f, 0.06185567f
};

// performs poorly on black 2d barcodes on white
const float Dib::DPI_400_KERNEL[9] = {
   0.0587031f, 0.1222315f, 0.0587031f,
   0.1222315f, 0.2762618f, 0.1222315f,
   0.0587031f, 0.1222315f, 0.0587031f
};

Dib::Dib() : pixels(NULL), isAllocated(false) {
}

Dib::Dib(const Dib & src)
      : pixels(NULL), isAllocated(false) {
   init(src.width, src.height, src.colorBits, src.pixelsPerMeter);
   memcpy(pixels, src.pixels, src.imageSize);
}

Dib::Dib(unsigned width, unsigned height, unsigned colorBits,
         unsigned pixelsPerMeter)
      : pixels(NULL), isAllocated(false) {
   init(width, height, colorBits, pixelsPerMeter);
}

void Dib::init(unsigned width, unsigned height, unsigned colorBits,
               unsigned pixelsPerMeter, bool allocatePixelBuf) {
   this->size = 40;
   this->width = width;
   this->height = height;
   this->colorBits = colorBits;
   this->bytesPerPixel = colorBits >> 3;
   this->pixelsPerMeter = pixelsPerMeter;
   this->paletteSize = getPaletteSize(colorBits);

   rowBytes = getRowBytes(width, colorBits);
   rowPaddingBytes = rowBytes - (width * bytesPerPixel);
   imageSize = height * rowBytes;

   if (allocatePixelBuf) {
      allocate(imageSize);
   }
   VLOG(5) << "constructor: image size is " << imageSize;
}

Dib::~Dib() {
   deallocate();
}

void Dib::allocate(unsigned int allocateSize) {
   isAllocated = true;
   pixels = new unsigned char[allocateSize];
   memset(pixels, 255, allocateSize);
   CHECK_NOTNULL(pixels);
}

void Dib::deallocate() {
   if (isAllocated && (pixels != NULL)) {
      delete[] pixels;
      pixels = NULL;
   }
}

void Dib::initPalette(RgbQuad * colorPalette) const {
   unsigned paletteSize = getPaletteSize(colorBits);
   CHECK(paletteSize != 0);

   CHECK_NOTNULL(colorPalette);
   for (unsigned i = 0; i < paletteSize; ++i) {
      colorPalette[i].set(i, i, i);
   }
}

unsigned Dib::getPaletteSize(unsigned colorBits) const {
   switch (colorBits) {
      case 1:
         return 2;
      case 4:
         return 16;
      case 8:
         return 256;
      default:
         return 0;
   }

}

void Dib::readFromHandle(HANDLE handle) {
#ifdef WIN32
   BITMAPINFOHEADER *dibHeaderPtr = (BITMAPINFOHEADER *) GlobalLock(handle);

   // if these conditions are not met the Dib cannot be processed
   CHECK(dibHeaderPtr->biSize == 40);
   CHECK(dibHeaderPtr->biPlanes == 1);
   CHECK(dibHeaderPtr->biCompression == 0);
   CHECK(dibHeaderPtr->biXPelsPerMeter == dibHeaderPtr->biYPelsPerMeter);
   CHECK(dibHeaderPtr->biClrImportant == 0);

   init(dibHeaderPtr->biWidth, dibHeaderPtr->biHeight,
        dibHeaderPtr->biBitCount, dibHeaderPtr->biXPelsPerMeter, false);

   pixels = reinterpret_cast <unsigned char *>(dibHeaderPtr)
      + sizeof(BITMAPINFOHEADER) + paletteSize * sizeof(RgbQuad);

   VLOG(2) << "readFromHandle: "
                   << " size/" << size
                   << " width/" << width
                   << " height/" << height
                   << " colorBits/" << colorBits
                   << " imageSize/" << imageSize
                   << " rowBytes/" << rowBytes
                   << " paddingBytes/" << rowPaddingBytes << " dpi/" << getDpi();
#endif
}

/**
 * All values in little-endian except for BitmapFileHeader.type.
 */
bool Dib::readFromFile(const string & filename) {
   deallocate();

   FILE *fh = fopen(filename.c_str(), "rb"); // C4996

   if (fh == NULL) {
      LOG(ERROR) << "could not open file " << filename;
      return false;
   }

   unsigned char fileHeaderRaw[0xE];
   unsigned char infoHeaderRaw[0x28];
   unsigned r;

   r = fread(fileHeaderRaw, sizeof(unsigned char), sizeof(fileHeaderRaw), fh);
   CHECK(r = sizeof(fileHeaderRaw));
   r = fread(infoHeaderRaw, sizeof(unsigned char), sizeof(infoHeaderRaw), fh);
   CHECK(r = sizeof(infoHeaderRaw));

   //unsigned size = *(unsigned *) &infoHeaderRaw[0];
   unsigned width = *(unsigned *) &infoHeaderRaw[0x12 - 0xE];
   unsigned height = *(unsigned *) &infoHeaderRaw[0x16 - 0xE];
   unsigned colorBits = *(unsigned short *) &infoHeaderRaw[0x1C - 0xE];
   unsigned hPixelsPerMeter = *(unsigned *) &infoHeaderRaw[0x26 - 0xE];
   unsigned vPixelsPerMeter = *(unsigned *) &infoHeaderRaw[0x2A - 0xE];

   unsigned planes = *(unsigned short *) &infoHeaderRaw[0x1A - 0xE];
   unsigned compression = *(unsigned *) &infoHeaderRaw[0x1E - 0xE];
   unsigned numColorsImp = *(unsigned *) &infoHeaderRaw[0x32 - 0xE];

   //FIXME this is required for gimp-based cropped images.
   // if these conditions are not met the Dib cannot be processed
   //CHECK(size == 40);

   CHECK(planes == 1);
   CHECK(compression == 0);
   CHECK(hPixelsPerMeter == vPixelsPerMeter);
   CHECK(numColorsImp == 0);

   init(width, height, colorBits, hPixelsPerMeter);

   r = fread(pixels, sizeof(unsigned char), imageSize, fh);
   CHECK(r = imageSize);
   fclose(fh);

   VLOG(2)
      << "readFromFile: rowBytes/" << rowBytes << " paddingBytes/"
      << rowPaddingBytes;
   return true;
}

bool Dib::writeToFile(const string & filename) const {
   CHECK_NOTNULL(pixels);

   unsigned char fileHeaderRaw[0xE];
   unsigned char infoHeaderRaw[0x28];
   unsigned paletteSize = getPaletteSize(colorBits);
   unsigned paletteBytes = paletteSize * sizeof(RgbQuad);

   *(unsigned short *) &fileHeaderRaw[0] = 0x4D42;
   *(unsigned *) &fileHeaderRaw[2] = imageSize + sizeof(fileHeaderRaw)
      + sizeof(infoHeaderRaw) + paletteBytes;
   *(unsigned short *) &fileHeaderRaw[6] = 0;
   *(unsigned short *) &fileHeaderRaw[8] = 0;
   *(unsigned *) &fileHeaderRaw[0xA] = sizeof(fileHeaderRaw)
      + sizeof(infoHeaderRaw) + paletteBytes;

   *(unsigned *) &infoHeaderRaw[0] = size;
   *(unsigned *) &infoHeaderRaw[0x12 - 0xE] = width;
   *(unsigned *) &infoHeaderRaw[0x16 - 0xE] = height;
   *(unsigned short *) &infoHeaderRaw[0x1A - 0xE] = 1;
   *(unsigned short *) &infoHeaderRaw[0x1C - 0xE] = colorBits;
   *(unsigned *) &infoHeaderRaw[0x1E - 0xE] = 0;
   *(unsigned *) &infoHeaderRaw[0x22 - 0xE] = imageSize;
   *(unsigned *) &infoHeaderRaw[0x26 - 0xE] = pixelsPerMeter;
   *(unsigned *) &infoHeaderRaw[0x2A - 0xE] = pixelsPerMeter;
   *(unsigned *) &infoHeaderRaw[0x2E - 0xE] = paletteSize;
   *(unsigned *) &infoHeaderRaw[0x32 - 0xE] = 0;

   FILE *fh = fopen(filename.c_str(), "wb"); // C4996
   if (fh == NULL) {
      DLOG(ERROR) << "could not open file for writing";
      return false;
   }

   unsigned r = fwrite(fileHeaderRaw, sizeof(unsigned char),
                       sizeof(fileHeaderRaw), fh);
   CHECK(r == sizeof(fileHeaderRaw));
   r = fwrite(infoHeaderRaw, sizeof(unsigned char), sizeof(infoHeaderRaw), fh);
   CHECK(r == sizeof(infoHeaderRaw));
   if (paletteSize > 0) {
      RgbQuad * colorPalette = new RgbQuad[paletteSize];
      initPalette(colorPalette);
      r = fwrite(colorPalette, sizeof(unsigned char), paletteBytes, fh);
      CHECK_EQ(r, paletteBytes);
      delete[] colorPalette;
   }
   r = fwrite(pixels, sizeof(unsigned char), imageSize, fh);
   CHECK_EQ(r, imageSize);
   fclose(fh);
   return true;
}

unsigned Dib::getRowBytes(unsigned width, unsigned colorBits) {
   return static_cast<unsigned>(ceil((width * colorBits) / 32.0)) << 2;
}

unsigned Dib::getDpi() const {
   // 1 inch = 0.0254 meters
   return static_cast<unsigned>(pixelsPerMeter * 0.0254 + 0.5);
}

unsigned Dib::getHeight() const {
   return height;
}

unsigned Dib::getWidth() const {
   return width;
}

unsigned Dib::getBitsPerPixel() const {
   return colorBits;
}

void Dib::setPixel(unsigned x, unsigned y, const RgbQuad & quad) {
   CHECK(x < width);
   CHECK(y < height);

   unsigned char *ptr = (pixels + y * rowBytes + x * bytesPerPixel);

   if (colorBits == 8) {
      RgbQuad scaledQuad(quad);
      scaledQuad.scale(0.3333);
      *ptr = scaledQuad.toUnsignedInt();
      return;
   }

   CHECK((colorBits == 24) || (colorBits == 32)) <<
      "can't assign RgbQuad to dib";

   ptr[2] = quad.getRed();
   ptr[1] = quad.getGreen();
   ptr[0] = quad.getBlue();
}

/*
 * [a,b)
 */
bool Dib::bound(unsigned min, unsigned & x, unsigned max) {
   bool valueChanged = false;

   if (x < min) {
      x = min;
      valueChanged = true;
   }
   if (x >= max) {
      x = max - 1;
      valueChanged = true;
   }
   return valueChanged;
}

std::tr1::shared_ptr<Dib> Dib::convertGrayscale() const {
   CHECK(colorBits == 24 || colorBits == 8);
   CHECK(pixels != NULL);

   if (getBitsPerPixel() == 8) {
      CHECK(false) << "already grayscale image.";
   }

   VLOG(2)
      << "convertGrayscale: Converting from 24 bit to 8 bit.";

   // 24bpp -> 8bpp
   Dib * dest = new Dib(width, height, 8, pixelsPerMeter);

   VLOG(2)
      << "convertGrayscale: Made dib";

   unsigned char *srcRowPtr = pixels;
   unsigned char *destRowPtr = dest->pixels;
   unsigned char *srcPtr, *destPtr;

   for (unsigned row = 0; row < height; ++row) {
      srcPtr = srcRowPtr;
      destPtr = destRowPtr;
      for (unsigned col = 0; col < width; ++col, ++destPtr) {
         *destPtr = static_cast<unsigned char>(0.3333 * srcPtr[0]
                                               + 0.3333 * srcPtr[1] + 0.3333 * srcPtr[2]);srcPtr +=bytesPerPixel;
      }

      // now initialise the padding bytes
      destPtr = destRowPtr + dest->width;
      for (unsigned i = 0; i < dest->rowPaddingBytes; ++i) {
         destPtr[i] = 0;
      }
      srcRowPtr += rowBytes;
      destRowPtr += dest->rowBytes;
   }

   VLOG(2)
      << "convertGrayscale: Generated 8 bit grayscale image.";

   return std::tr1::shared_ptr<Dib>(dest);
}

/*
 * DIBs are flipped in Y
 *
 * TODO At the moment crops are only done on the bounding box. In the future
 * any rectangle, at any angle, should be allowed.
 */
std::tr1::shared_ptr<Dib> Dib::crop(unsigned x0, unsigned y0, unsigned x1,
                                    unsigned y1) const {
   CHECK(x1 > x0);
   CHECK(y1 > y0);

   bound(0, x0, width);
   bound(0, x1, width);
   bound(0, y0, height);
   bound(0, y1, height);

   unsigned cWidth = x1 - x0;
   unsigned cHeight = y1 - y0;

   std::tr1::shared_ptr<Dib> croppedImg(
      new Dib(cWidth, cHeight, colorBits, pixelsPerMeter));

   unsigned char *srcRowPtr = pixels + (height - y1) * rowBytes
      + x0 * croppedImg->bytesPerPixel;
   unsigned char *destRowPtr = croppedImg->pixels;
   unsigned row = 0;

   while (row < cHeight) {
      memcpy(destRowPtr, srcRowPtr, croppedImg->rowBytes);
      memset(destRowPtr + croppedImg->rowBytes - croppedImg->rowPaddingBytes,
             0, croppedImg->rowPaddingBytes);

      ++row;
      srcRowPtr += rowBytes;
      destRowPtr += croppedImg->rowBytes;
   }
   return croppedImg;
}

/*
 * generate x,y coordinates to draw a line from x0,y0 to x1,y1 and output the
 * coordinates in the same direction that they are drawn.
 *
 * any coordinates which overlap other coordinates are duplicates and are
 * removed from the output because they are redundant.
 *
 * Taken from: http://en.wikipedia.org/wiki/Bresenham's_line_algorithm
 */
void Dib::drawLine(unsigned x0, unsigned y0, unsigned x1, unsigned y1,
               const RgbQuad & quad) {
   CHECK(y0 < height);
   CHECK(y1 < height);
   CHECK(x0 < width);
   CHECK(x1 < width);

   unsigned x, deltax, y, deltay, st;
   int cx, cy, error, xstep, ystep;

   y0 = height - y0 - 1;
   y1 = height - y1 - 1;

   // find largest delta for pixel steps
   deltax = abs(static_cast<int>(x1) - static_cast<int>(x0));
   deltay = abs(static_cast<int>(y1) - static_cast<int>(y0));

   st = deltay > deltax;

   // if deltay > deltax then swap x,y
   if (st) {
      x0 ^= y0;
      y0 ^= x0;
      x0 ^= y0; // swap(x0, y0);
      x1 ^= y1;
      y1 ^= x1;
      x1 ^= y1; // swap(x1, y1);
   }

   deltax = abs(static_cast<int>(x1) - static_cast<int>(x0));
   deltay = abs(static_cast<int>(y1) - static_cast<int>(y0));
   error = (deltax / 2);
   y = y0;

   if (x0 > x1) {
      xstep = -1;
   } else {
      xstep = 1;
   }

   if (y0 > y1) {
      ystep = -1;
   } else {
      ystep = 1;
   }

   for (x = x0; x != x1 + xstep; x += xstep) {
      cx = x;
      cy = y; // copy of x, copy of y

      // if x,y swapped above, swap them back now
      if (st) {
         cx ^= cy;
         cy ^= cx;
         cx ^= cy;
      }

      setPixel(cx, cy, quad);
      error -= deltay; // converge toward end of line

      if (error < 0) { // not done yet
         y += ystep;
         error += deltax;
      }
   }
}

void Dib::drawRectangle(const Rect<unsigned> & rect, const RgbQuad & quad) {
   drawLine(rect.corners[0], rect.corners[1], quad);
   drawLine(rect.corners[1], rect.corners[2], quad);
   drawLine(rect.corners[2], rect.corners[3], quad);
   drawLine(rect.corners[3], rect.corners[0], quad);
}

void Dib::drawRectangle(unsigned x, unsigned y, unsigned width, unsigned height,
                    const RgbQuad & quad) {
   drawLine(x, y, x, y + height, quad);
   drawLine(x, y, x + width, y, quad);
   drawLine(x + width, y, x + width, y + height, quad);
   drawLine(x, y + height, x + width, y + height, quad);
}

void Dib::tpPresetFilter() {
   switch (getDpi()) {

      case 400:
         VLOG(2)
            << "tpPresetFilter: Applying DPI_400_KERNEL";
         convolveFast3x3(Dib::DPI_400_KERNEL);
         break;

      case 600:
         VLOG(2)
            << "tpPresetFilter: Applying BLANK_KERNEL";
         convolveFast3x3(Dib::BLANK_KERNEL);

         VLOG(2)
            << "tpPresetFilter: Applying BLUR_KERNEL";
         convolveFast3x3(Dib::BLUR_KERNEL);
         break;

      case 300:
         VLOG(2)
            << "tpPresetFilter: No filter applied (300 dpi)";
         break;

      default:
         VLOG(2)
            << "tpPresetFilter: No filter applied (default) dpi/" << getDpi();
         break;
   }
}

// Can only be used for grayscale Dibs
void Dib::convolveFast3x3(const float(&k)[9]) {
   CHECK_EQ(colorBits, (unsigned) 8)
      << "convolveFast3x3 requires an unsigned 8bit image";

   unsigned size = height * width;

   float *imageOut = new float[size];
   float *imageIn = new float[size];

   fill(imageOut, imageOut + size, 0.0f);

   {
      unsigned char *srcRowPtr = pixels, *srcPtr;
      float *destPtr = imageIn;

      for (unsigned row = 0; row < height; row++) {
         srcPtr = srcRowPtr;
         for (unsigned col = 0; col < width; col++, srcPtr++, destPtr++) {
            *destPtr = *srcPtr;
         }
         srcRowPtr += rowBytes;
      }
   }

   int ncm1 = width - 1, nrm1 = height - 1;
   float k00 = k[0];
   float k01 = k[1];
   float k02 = k[2];
   float k10 = k[3];
   float k11 = k[4];
   float k12 = k[5];
   float k20 = k[6];
   float k21 = k[7];
   float k22 = k[8];
   for (int i = 1; i < nrm1; i++) {
      float *r00 = imageIn + (i - 1) * width;
      float *r01 = r00 + 1;
      float *r02 = r01 + 1;
      float *r10 = r00 + width;
      float *r11 = r10 + 1;
      float *r12 = r11 + 1;
      float *r20 = r10 + width;
      float *r21 = r20 + 1;
      float *r22 = r21 + 1;
      float *rOut = imageOut + i * width + 1;
      for (int j = 1; j < ncm1; j++) {
         *rOut++ = (k00 * *r00++) + (k01 * *r01++) + (k02 * *r02++)
            + (k10 * *r10++) + (k11 * *r11++) + (k12 * *r12++)
            + (k20 * *r20++) + (k21 * *r21++) + (k22 * *r22++);
      }
   }

   {
      float *srcPtr = imageOut;
      unsigned char *destRowPtr = pixels, *destPtr;

      for (unsigned row = 0; row < height; row++) {
         destPtr = destRowPtr;
         for (unsigned col = 0; col < width; col++, srcPtr++, destPtr++) {
            *destPtr = static_cast<unsigned char>(*srcPtr);
         }
         destRowPtr += rowBytes;
      }
   }

   delete[] imageIn;
   delete[] imageOut;
}

/**
 * The caller must detroy the image.
 */
DmtxImage * Dib::getDmtxImage() const {
   int pack = DmtxPackCustom;

   switch (colorBits) {
      case 8:
         pack = DmtxPack8bppK;
         break;
      case 24:
         pack = DmtxPack24bppRGB;
         break;
      case 32:
         pack = DmtxPack32bppXRGB;
         break;
   }

   DmtxImage * image =  dmtxImageCreate(pixels, width, height, pack);

   //set the properties (pad bytes, flip)
   dmtxImageSetProp(image, DmtxPropRowPadBytes, rowPaddingBytes);
   dmtxImageSetProp(image, DmtxPropImageFlip, DmtxFlipY); // DIBs are flipped in Y
   return image;
}
