#ifndef __INCLUDE_DIB_H
#define __INCLUDE_DIB_H

/*
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
 */

#include "cv.h"
#include "cvblob/BlobResult.h"
#include "IplContainer.h"

#ifdef WIN32
#   include <windows.h>
#else
typedef void* HANDLE;
#endif

#include <memory>

#ifndef _VISUALC_
#   include <tr1/memory>
#endif

using namespace std;

/* Colour palette
 */
struct RgbQuad {

	RgbQuad();
	RgbQuad(unsigned char r, unsigned char g, unsigned char b);
	void set(unsigned char r, unsigned char g, unsigned char b);

	unsigned char rgbRed;
	unsigned char rgbGreen;
	unsigned char rgbBlue;
	unsigned char rgbReserved;
};

class Dib {
public:

	Dib();
	Dib(const Dib & src);
	Dib(IplImageContainer & src);
	Dib(unsigned width, unsigned height, unsigned colorBits,
			unsigned pixelsPerMeter);
	Dib(char * filename);

	~Dib();

	void readFromFile(const char * filename);
	bool writeToFile(const char * filename) const;

	std::tr1::shared_ptr<Dib> convertGrayscale() const;

	std::tr1::shared_ptr<Dib> crop(unsigned x0, unsigned y0, unsigned x1,
			unsigned y1) const;

	auto_ptr<IplImageContainer> generateIplImage();
	void tpPresetFilter();

	unsigned getDpi() const;
	unsigned getHeight() const;
	unsigned getWidth() const;
	unsigned getRowPadBytes() const;
	unsigned getBitsPerPixel() const;
	unsigned char * getPixelBuffer() const;
	unsigned char getPixelAvgGrayscale(unsigned row, unsigned col) const;
	inline unsigned char getPixelGrayscale(unsigned row, unsigned col) const;

	void setPixel(unsigned row, unsigned col, RgbQuad & quad);
	inline void setPixelGrayscale(unsigned row, unsigned col,
			unsigned char value);

	void line(unsigned x0, unsigned y0, unsigned x1, unsigned y1,
			RgbQuad & quad);

	void line(const CvPoint & start, const CvPoint & end, RgbQuad & quad) {
	    line(start.x, start.y, end.x, end.y, quad);
	}

	void rectangle(unsigned x, unsigned y, unsigned width, unsigned height,
			RgbQuad & quad);

	void readFromHandle(HANDLE handle);

private:

	static const float BLUR_KERNEL[9];
	static const float BLANK_KERNEL[9];
	static const float DPI_400_KERNEL[9];
	static const double UNSHARP_RAD;
	static const double UNSHARP_DEPTH;
	static const unsigned GAUSS_WIDTH;
	static const unsigned GAUSS_FACTORS[];
	static const unsigned GAUSS_SUM;

	unsigned size;
	unsigned width;
	unsigned imageSize;
	unsigned height;
	unsigned colorBits;
	unsigned pixelsPerMeter;
	unsigned paletteSize;
	unsigned bytesPerPixel;
	unsigned rowBytes;
	unsigned rowPaddingBytes;
	unsigned char * pixels;

	/*
	 * Dib can be created with a borrowed pixel buffer. When an image is scanned,
	 * the TWAIN driver owns the memory to the pixel buffer. In this case
	 * isAllocated is set to false.
	 */
	bool isAllocated;

	void init(unsigned width, unsigned height, unsigned colorBits,
			unsigned pixelsPerMeter, bool allocatePixelBuf = true);
	void initPalette(RgbQuad * palette) const;

	void allocate(unsigned int allocateSize);
	void deallocate();

	unsigned getPaletteSize(unsigned bitCount) const;
	unsigned getRowBytes(unsigned width, unsigned bitCount);
	static bool bound(unsigned min, unsigned & x, unsigned max);
	void convolveFast3x3(const float(&kernel)[9]);

};

#endif /* __INCLUDE_DIB_H */
