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
#else
//#include <strings.h>
#endif

#include "Dib.h"
#include "UaLogger.h"
#include "UaAssert.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>

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
const unsigned Dib::GAUSS_FACTORS[GAUSS_WIDTH] = { 1, 11, 55, 165, 330, 462,
		462, 330, 165, 55, 11, 1 };

const unsigned Dib::GAUSS_SUM = 2048;

// expects sharp image
const float Dib::BLUR_KERNEL[9] = { 0.0f, 0.2f, 0.0f, 0.2f, 0.2f, 0.2f, 0.0f,
		0.2f, 0.0f, };

// expects sharp image
const float Dib::BLANK_KERNEL[9] = { 0.06185567f, 0.12371134f, 0.06185567f,
		0.12371134f, 0.257731959f, 0.12371134f, 0.06185567f, 0.12371134f,
		0.06185567f, };

// performs poorly on black 2d barcodes on white
const float Dib::DPI_400_KERNEL[9] =
		{ 0.0587031f, 0.1222315f, 0.0587031f, 0.1222315f, 0.2762618f,
				0.1222315f, 0.0587031f, 0.1222315f, 0.0587031f, };


RgbQuad::RgbQuad() {
	set(0, 0, 0);
}

RgbQuad::RgbQuad(unsigned char r, unsigned char g, unsigned char b) {
	set(r, g, b);
}

void RgbQuad::set(unsigned char r, unsigned char g, unsigned char b) {
	rgbRed = r;
	rgbGreen = g;
	rgbBlue = b;
	rgbReserved = 0;
}



Dib::Dib() :
		pixels(NULL) {
	ua::Logger::Instance().subSysHeaderSet(4, "Dib");
}

Dib::Dib(Dib & src) :
		pixels(NULL) {
	ua::Logger::Instance().subSysHeaderSet(4, "Dib");
	init(src.width, src.height, src.colorBits, src.pixelsPerMeter);
	memcpy(pixels, src.pixels, src.width * src.height);
}

Dib::Dib(unsigned width, unsigned height, unsigned colorBits,
		unsigned pixelsPerMeter) {
	init(width, height, colorBits, pixelsPerMeter);
}

Dib::Dib(IplImageContainer & img) {
	IplImage *src = img.getIplImage();

	UA_ASSERTS(src->depth == IPL_DEPTH_8U,
			"Dib::Dib(IplImage & src) requires an unsigned 8bit image");

	CvMat hdr, *matrix = NULL;
	/* IplImage saves a flipped image */
	cvFlip(src, src, 0);
	matrix = cvGetMat(src, &hdr);

	init(src->width, src->height, 8, img.getHorizontalResolution());
	memcpy(pixels, matrix->data.ptr, src->imageSize);
	cvFlip(src, src, 0);
}

Dib::Dib(char *filename) :
		pixels(NULL) {
	readFromFile(filename);
}

void Dib::init(unsigned width, unsigned height, unsigned colorBits,
		unsigned pixelsPerMeter) {

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

	allocate(imageSize);

	UA_DOUT(4, 5, "constructor: image size is " << imageSize);
}

Dib::~Dib() {
	deallocate();
}

void Dib::allocate(unsigned int allocateSize) {
	pixels = new unsigned char[allocateSize];
	memset(pixels, 255, allocateSize);
	UA_ASSERT_NOT_NULL(pixels);
}

void Dib::deallocate() {
	if (pixels != NULL) {
		delete[] pixels;
		pixels = NULL;
	}
}

void Dib::initPalette(RgbQuad * colorPalette) const {
	unsigned paletteSize = getPaletteSize(colorBits);
	UA_ASSERT(paletteSize != 0);

	UA_ASSERT_NOT_NULL(colorPalette);
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

#ifdef WIN32
void Dib::readFromHandle(HANDLE handle) {
	BITMAPINFOHEADER *dibHeaderPtr = (BITMAPINFOHEADER *) GlobalLock(handle);

	// if these conditions are not met the Dib cannot be processed
	UA_ASSERT(dibHeaderPtr->biSize == 40);
	UA_ASSERT(dibHeaderPtr->biPlanes == 1);
	UA_ASSERT(dibHeaderPtr->biCompression == 0);
	UA_ASSERT(dibHeaderPtr->biXPelsPerMeter == dibHeaderPtr->biYPelsPerMeter);
	UA_ASSERT(dibHeaderPtr->biClrImportant == 0);

	init(dibHeaderPtr->biWidth, dibHeaderPtr->biHeight,
			dibHeaderPtr->biBitCount, dibHeaderPtr->biXPelsPerMeter);

	pixels = reinterpret_cast <unsigned char *>(dibHeaderPtr)
	+ sizeof(BITMAPINFOHEADER) + paletteSize * sizeof(RgbQuad);

	UA_DOUT(4, 5, "readFromHandle: "
			<< " size/" << size
			<< " width/" << width
			<< " height/" << height
			<< " colorBits/" << colorBits
			<< " imageSize/" << imageSize
			<< " rowBytes/" << rowBytes
			<< " paddingBytes/" << rowPaddingBytes << " dpi/" << getDpi());
}
#endif

/**
 * All values in little-endian except for BitmapFileHeader.type.
 */
void Dib::readFromFile(const char *filename) {
	UA_ASSERT_NOT_NULL(filename);

	deallocate();

	FILE *fh = fopen(filename, "rb"); // C4996
	if (fh == NULL) {
		UA_ERROR("could not open file " << filename);
	}

	unsigned char fileHeaderRaw[0xE];
	unsigned char infoHeaderRaw[0x28];
	unsigned r;

	r = fread(fileHeaderRaw, sizeof(unsigned char), sizeof(fileHeaderRaw), fh);
	UA_ASSERT(r = sizeof(fileHeaderRaw));
	r = fread(infoHeaderRaw, sizeof(unsigned char), sizeof(infoHeaderRaw), fh);
	UA_ASSERT(r = sizeof(infoHeaderRaw));

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
	//UA_ASSERT(size == 40);

	UA_ASSERT(planes == 1);
	UA_ASSERT(compression == 0);
	UA_ASSERT(hPixelsPerMeter == vPixelsPerMeter);
	UA_ASSERT(numColorsImp == 0);

	init(width, height, colorBits, hPixelsPerMeter);

	r = fread(pixels, sizeof(unsigned char), imageSize, fh);
	UA_ASSERT(r = imageSize);
	fclose(fh);

	UA_DOUT(
			4,
			5,
			"readFromFile: rowBytes/" << rowBytes << " paddingBytes/" << rowPaddingBytes);
}

unsigned Dib::getRowBytes(unsigned width, unsigned colorBits) {
	return static_cast<unsigned>(ceil((width * colorBits) / 32.0)) << 2;
}

bool Dib::writeToFile(const char *filename) const {
	UA_ASSERT_NOT_NULL(filename);
	UA_ASSERT_NOT_NULL(pixels);

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

	FILE *fh = fopen(filename, "wb"); // C4996
	if (fh == NULL) {
		// could not open file for writing
		return false;
	}

	unsigned r = fwrite(fileHeaderRaw, sizeof(unsigned char),
			sizeof(fileHeaderRaw), fh);
	UA_ASSERT(r == sizeof(fileHeaderRaw));
	r = fwrite(infoHeaderRaw, sizeof(unsigned char), sizeof(infoHeaderRaw), fh);
	UA_ASSERT(r == sizeof(infoHeaderRaw));
	if (paletteSize > 0) {
		RgbQuad * colorPalette = new RgbQuad[paletteSize];
		initPalette(colorPalette);
		r = fwrite(colorPalette, sizeof(unsigned char), paletteBytes, fh);
		UA_ASSERT(r == paletteBytes);
		delete[] colorPalette;
	}
	r = fwrite(pixels, sizeof(unsigned char), imageSize, fh);
	UA_ASSERT(r == imageSize);
	fclose(fh);
	return true;
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

unsigned Dib::getRowPadBytes() const {
	return rowPaddingBytes;
}

unsigned Dib::getBitsPerPixel() const {
	return colorBits;
}

unsigned char *Dib::getPixelBuffer() const {
	UA_ASSERT_NOT_NULL(pixels);
	return pixels;
}

unsigned char Dib::getPixelAvgGrayscale(unsigned row, unsigned col) const {
	UA_ASSERT(row < height);
	UA_ASSERT(col < width);

	unsigned char *ptr = pixels + row * rowBytes + col * bytesPerPixel;

	if ((colorBits == 24) || (colorBits == 32)) {
		return (unsigned char) (0.33333 * ptr[0] + 0.33333 * ptr[1]
				+ 0.33333 * ptr[2]);
	} else if (colorBits == 8) {
		return *ptr;
	}UA_ASSERTS(false, "colorBits " << colorBits << " not implemented yet");
	return 0;
}

inline unsigned char Dib::getPixelGrayscale(unsigned row, unsigned col) const {
	UA_ASSERT(row < height);
	UA_ASSERT(col < width);

	unsigned char *ptr = pixels + row * rowBytes + col * bytesPerPixel;

	if ((colorBits == 24) || (colorBits == 32)) {
		return static_cast<unsigned char>(0.3333 * ptr[0] + 0.3333 * ptr[1]
				+ 0.3333 * ptr[2]);
	} else if (colorBits == 8) {
		return *ptr;
	}UA_ASSERTS(false, "colorBits " << colorBits << " not implemented yet");
	return 0;
}

void Dib::setPixel(unsigned x, unsigned y, RgbQuad & quad) {
	UA_ASSERT(x < width);
	UA_ASSERT(y < height);

	unsigned char *ptr = (pixels + y * rowBytes + x * bytesPerPixel);

	if (colorBits == 8) {
		*ptr = static_cast<unsigned char>(0.3333 * quad.rgbRed
				+ 0.3333 * quad.rgbGreen + 0.3333 * quad.rgbBlue);return;
	}

	if ((colorBits != 24) && (colorBits != 32)) {
		UA_ERROR("can't assign RgbQuad to dib");
	}

	ptr[2] = quad.rgbRed;
	ptr[1] = quad.rgbGreen;
	ptr[0] = quad.rgbBlue;
}

inline void Dib::setPixelGrayscale(unsigned row, unsigned col,
		unsigned char value) {
	UA_ASSERT(row < height);
	UA_ASSERT(col < width);

	unsigned char *ptr = pixels + row * rowBytes + col * bytesPerPixel;

	if ((colorBits == 24) || (colorBits == 32)) {
		ptr[0] = value;
		ptr[1] = value;
		ptr[2] = value;
	} else if (colorBits == 8) {
		*ptr = value;
	} else {
		assert(0);
		/* can't assign RgbQuad to dib */
	}
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

/*
 * DIBs are flipped in Y
 */
Dib * Dib::crop(Dib & src, unsigned x0, unsigned y0, unsigned x1, unsigned y1) {
	UA_ASSERT(x1 > x0);
	UA_ASSERT(y1 > y0);

	bound(0, x0, src.width);
	bound(0, x1, src.width);
	bound(0, y0, src.height);
	bound(0, y1, src.height);

	unsigned width = x1 - x0;
	unsigned height = y1 - y0;

	Dib * dest = new Dib(width, height, src.colorBits, src.pixelsPerMeter);

	unsigned char *srcRowPtr = src.pixels + (src.height - y1) * src.rowBytes
			+ x0 * dest->bytesPerPixel;
	unsigned char *destRowPtr = dest->pixels;
	unsigned row = 0;

	while (row < height) {
		memcpy(destRowPtr, srcRowPtr, width);
		memset(destRowPtr + width, 0, dest->rowPaddingBytes);

		++row;
		srcRowPtr += src.rowBytes;
		destRowPtr += dest->rowBytes;
	}
	return dest;
}

auto_ptr<Dib> Dib::convertGrayscale(Dib & src) {
	UA_ASSERT(src.getBitsPerPixel() == 24 || src.getBitsPerPixel() == 8);
	UA_ASSERT(src.getPixelBuffer() != NULL);

	if (src.getBitsPerPixel() == 8) {
		UA_DOUT(4, 9, "convertGrayscale: Already grayscale image.");
		return auto_ptr<Dib>(&src);
	}

	UA_DOUT(4, 9, "convertGrayscale: Converting from 24 bit to 8 bit.");

	// 24bpp -> 8bpp
	auto_ptr<Dib> dest(new Dib(src.width, src.height, 8, src.pixelsPerMeter));

	UA_DOUT(4, 9, "convertGrayscale: Made dib");

	unsigned char *srcRowPtr = src.pixels;
	unsigned char *destRowPtr = dest->pixels;
	unsigned char *srcPtr, *destPtr;

	for (unsigned row = 0; row < src.height; ++row) {
		srcPtr = srcRowPtr;
		destPtr = destRowPtr;
		for (unsigned col = 0; col < src.width; ++col, ++destPtr) {
			*destPtr = static_cast<unsigned char>(0.3333 * srcPtr[0]
					+ 0.3333 * srcPtr[1] + 0.3333 * srcPtr[2]);srcPtr += src.bytesPerPixel;
		}

			// now initialise the padding bytes
		destPtr = destRowPtr + dest->width;
		for (unsigned i = 0; i < dest->rowPaddingBytes; ++i) {
			destPtr[i] = 0;
		}
		srcRowPtr += src.rowBytes;
		destRowPtr += dest->rowBytes;
	}

	UA_DOUT(4, 9, "convertGrayscale: Generated 8 bit grayscale image.");

	return dest;
}

/*
 * /http://opencv.willowgarage.com/documentation/c/basic_structures.html
 *
 * cvmat: Matrices are stored row by row. All of the rows are padded (4 bytes).
 */
auto_ptr<IplImageContainer> Dib::generateIplImage() {
	UA_ASSERTS(colorBits == 8,
			"generateIplImage requires an unsigned 8bit image");
	UA_ASSERTS(pixels != NULL, "NULL pixel data specified to generateIplImage");

	IplImage *image = NULL;
	CvMat hdr, *matrix = NULL;
	CvSize size;

	size.width = width;
	size.height = height;

	image = cvCreateImage(size, IPL_DEPTH_8U, 1);
	matrix = cvGetMat(image, &hdr);

	memcpy(matrix->data.ptr, pixels, imageSize);

	cvFlip(image, image, 0);

	auto_ptr<IplImageContainer> iplContainer(new IplImageContainer(NULL));
	iplContainer->setIplImage(image);
	iplContainer->setHorizontalResolution(pixelsPerMeter);
	iplContainer->setVerticalResolution(pixelsPerMeter);
	return iplContainer;
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
void Dib::line(unsigned x0, unsigned y0, unsigned x1, unsigned y1,
		RgbQuad & quad) {
	UA_ASSERT(y0 < height);
	UA_ASSERT(y1 < height);
	UA_ASSERT(x0 < width);
	UA_ASSERT(x1 < width);

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

void Dib::rectangle(unsigned x, unsigned y, unsigned width, unsigned height,
		RgbQuad & quad) {
	line(x, y, x, y + height, quad);
	line(x, y, x + width, y, quad);
	line(x + width, y, x + width, y + height, quad);
	line(x, y + height, x + width, y + height, quad);

}

void Dib::tpPresetFilter() {
	switch (getDpi()) {

	case 400:
		UA_DOUT(4, 5, "tpPresetFilter: Applying DPI_400_KERNEL");
		convolveFast3x3(Dib::DPI_400_KERNEL);
		break;

	case 600:
		UA_DOUT(4, 5, "tpPresetFilter: Applying BLANK_KERNEL");
		convolveFast3x3(Dib::BLANK_KERNEL);

		UA_DOUT(4, 5, "tpPresetFilter: Applying BLUR_KERNEL");
		convolveFast3x3(Dib::BLUR_KERNEL);
		break;

	case 300:
		UA_DOUT(4, 5, "tpPresetFilter: No filter applied (300 dpi)");
		break;

	default:
		UA_DOUT(4, 5,
				"tpPresetFilter: No filter applied (default) dpi/" << getDpi());
		break;
	}
}

// Can only be used for grayscale Dibs
void Dib::convolveFast3x3(const float(&k)[9]) {
	UA_ASSERTS(colorBits == 8,
			"convolveFast3x3 requires an unsigned 8bit image");

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
