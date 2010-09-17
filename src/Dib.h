#ifndef __INCLUDE_DIB_H
#define __INCLUDE_DIB_H
/*
Dmscanlib is a software library and standalone application that scans 
and decodes libdmtx compatible test-tubes. It is currently designed 
to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
Copyright (C) 2010 Canadian Biosample Repository

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cv.h"
#include "BlobResult.h"
#include "IplContainer.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <memory>

using namespace std;

/* File information header
 * provides general information about the file
 */
struct BitmapFileHeader {
	unsigned short type;
	unsigned       size;
	unsigned short reserved1;
	unsigned short reserved2;
	unsigned       offset;
};

/* Bitmap information header
 * provides information specific to the image data
 */
struct BitmapInfoHeader{
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
};

/* Colour palette
 */
struct RgbQuad {
	RgbQuad() {
		set(0, 0, 0);
	}
	RgbQuad(unsigned char r, unsigned char g, unsigned char b) {
		set(r, g, b);
	}
	void set(unsigned char r, unsigned char g, unsigned char b) {
		rgbRed = r; rgbGreen = g; rgbBlue = b; rgbReserved = 0;
	}
	unsigned char rgbRed;
	unsigned char rgbGreen;
	unsigned char rgbBlue;
	unsigned char rgbReserved;
};

class Dib {
public:

	Dib();
	Dib(Dib & src);
	Dib(IplImageContainer & src);
	Dib(unsigned width, unsigned height, unsigned colorBits,
			unsigned pixelsPerMeter);
	Dib(char * filename);
	~Dib();
	void readFromFile(const char * filename) ;
	bool writeToFile(const char * filename);

	static auto_ptr<Dib> convertGrayscale(Dib & src);
	static auto_ptr<Dib> crop(Dib &src, unsigned x0, unsigned y0, unsigned x1, unsigned y1);

	void tpPresetFilter();

#ifdef WIN32
	void readFromHandle(HANDLE handle);
#endif
	void init(unsigned width, unsigned height, unsigned colorBits,
			unsigned pixelsPerMeter);
	unsigned getHeight();
	unsigned getWidth();
	unsigned getRowPadBytes();
	unsigned getInternalRowBytes();
	unsigned getBitsPerPixel();
	unsigned char * getPixelBuffer();
	unsigned char * getRowPtr(unsigned row);
	void getPixel(unsigned row, unsigned col, RgbQuad & quad);
	unsigned char getPixelAvgGrayscale(unsigned row, unsigned col);
	inline unsigned char getPixelGrayscale(unsigned row, unsigned col);
	void setPixel(unsigned row, unsigned col, RgbQuad & quad);
	inline void setPixelGrayscale(unsigned row, unsigned col, unsigned char value);
	unsigned char * getPixelsNoPadding();
	void setPixelsNoPadding(unsigned char * pixels);

	void line(unsigned x0, unsigned y0, unsigned x1, unsigned y1, RgbQuad & quad);
	void rectangle(unsigned x, unsigned y, unsigned width, unsigned height,RgbQuad & quad);
	unsigned getDpi();

	auto_ptr<IplImageContainer>  generateIplImage();
private:

	static const float BLUR_KERNEL[9];
	static const float BLANK_KERNEL[9];
	static const float DPI_400_KERNEL[9];
	static const double UNSHARP_RAD;
    static const double UNSHARP_DEPTH;
    static const unsigned GAUSS_WIDTH;
    static const unsigned GAUSS_FACTORS[];
    static const unsigned GAUSS_SUM;



	auto_ptr<BitmapInfoHeader> infoHeader;
	RgbQuad * colorPalette;
	unsigned bytesPerPixel;
	unsigned rowBytes;
	unsigned rowPaddingBytes;
	unsigned char * pixels;
	bool isAllocated;

	unsigned idx, dupe; // used by line drawing

	void deallocate();
	unsigned getPaletteSize(unsigned bitCount);
	void setPalette();
	void setPalette(RgbQuad * palette);
	unsigned getRowBytes(unsigned width, unsigned bitCount);

	void convolveFast3x3(const float(&kernel)[9]);
	void convolve2DSlow(const float(&kernel) [9], int kernelSizeX, int kernelSizeY);
	static bool bound(unsigned min, unsigned & x, unsigned max);
};


#endif /* __INCLUDE_DIB_H */
