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
/*******************************************************************************
 * Device Independent Bitmap
 ******************************************************************************/

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

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#else
#include <strings.h>
#endif

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

Dib::Dib():
	fileHeader(NULL), infoHeader(NULL), colorPalette(NULL), pixels(NULL),
			isAllocated(false) {
	ua::Logger::Instance().subSysHeaderSet(4, "Dib");
}

Dib::Dib(Dib & src):
	fileHeader(NULL), infoHeader(NULL), colorPalette(NULL), pixels(NULL),
			isAllocated(false) {
	ua::Logger::Instance().subSysHeaderSet(4, "Dib");
	copyInternals(src);
	memcpy(pixels, src.pixels, infoHeader->imageSize);
}

Dib::Dib(IplImageContainer & img) {

	IplImage *src = img.getIplImage();

	UA_ASSERTS(src->depth == IPL_DEPTH_8U,
			"Dib::Dib(IplImage & src) requires an unsigned 8bit image");

	CvMat hdr, *matrix = NULL;
	/* IplImage saves a flipped image */
	cvFlip(src, src, 0);
	matrix = cvGetMat(src, &hdr);

	bytesPerPixel = 1;
	rowBytes = src->widthStep;
	rowPaddingBytes = src->widthStep - (src->width * 1);

	fileHeader = NULL;
	infoHeader = new BitmapInfoHeader;

	infoHeader->size = 40; /* bytes in header should be 40 */
	infoHeader->width = src->width;
	infoHeader->height = src->height;
	infoHeader->planes = 1;
	infoHeader->bitCount = 8;
	infoHeader->compression = 0;
	infoHeader->hPixelsPerMeter = img.getHorizontalResolution();
	infoHeader->vPixelsPerMeter = img.getVerticalResolution();
	infoHeader->numColors = 256;
	infoHeader->numColorsImp = 0;
	infoHeader->imageSize = src->imageSize;

	colorPalette = new RgbQuad[256];
	for (unsigned i = 0; i < 256; ++i) {
		colorPalette[i].set(i, i, i);
	}

	/*---------pixels-----------*/
	isAllocated = true;
	pixels = new unsigned char[src->imageSize];

	/*IplImage is already padded */
	memcpy(pixels, matrix->data.ptr, src->imageSize);
	/*---------pixels-----------*/

	cvFlip(src, src, 0);
}

Dib::Dib(unsigned width, unsigned height, unsigned colorBits) :
	fileHeader(NULL), colorPalette(NULL) {
	bytesPerPixel = colorBits >> 3;

	unsigned paletteSize = getPaletteSize(colorBits);

	infoHeader = new BitmapInfoHeader;
	infoHeader->size = 40;
	infoHeader->width = width;
	infoHeader->height = height;
	infoHeader->planes = 1;
	infoHeader->bitCount = colorBits;
	infoHeader->compression = 0;
	infoHeader->hPixelsPerMeter = 0;
	infoHeader->vPixelsPerMeter = 0;
	infoHeader->numColors = paletteSize;
	infoHeader->numColorsImp = 0;

	if (paletteSize > 0) {
		colorPalette = new RgbQuad[paletteSize];
		setPalette();
	}

	rowBytes = getRowBytes(infoHeader->width, infoHeader->bitCount);
	rowPaddingBytes = rowBytes - (infoHeader->width * bytesPerPixel);
	infoHeader->imageSize = infoHeader->height * rowBytes;

	isAllocated = true;
	pixels = new unsigned char[infoHeader->imageSize];
	//memset(pixels, 255, infoHeader->imageSize);
	UA_DOUT(4, 5, "constructor: image size is " << infoHeader->imageSize);
}

Dib::Dib(char *filename) :
	fileHeader(NULL), infoHeader(NULL), pixels(NULL), isAllocated(false) {
	readFromFile(filename);
}

Dib::~Dib() {
	deallocate();
}

void Dib::deallocate() {
	if (fileHeader != NULL) {
		delete fileHeader;
	}
	if (infoHeader != NULL) {
		delete infoHeader;
	}

	if (colorPalette != NULL) {
		delete[] colorPalette;
	}

	if (isAllocated && (pixels != NULL)) {
		delete[] pixels;
	}
}

void Dib::setPalette() {
	unsigned paletteSize = getPaletteSize(infoHeader->bitCount);
	if (paletteSize == 0)
		return;

	UA_ASSERT_NOT_NULL(colorPalette);
	for (unsigned i = 0; i < paletteSize; ++i) {
		colorPalette[i].set(i, i, i);
	}
}

void Dib::setPalette(RgbQuad * palette) {
	unsigned paletteSize = getPaletteSize(infoHeader->bitCount);
	if (paletteSize == 0)
		return;
	UA_ASSERT_NOT_NULL(colorPalette);
	memcpy(colorPalette, palette, paletteSize * sizeof(RgbQuad));
}

void Dib::copyInternals(Dib & src) {
	if ((fileHeader != NULL) && (src.fileHeader != NULL)) {
		*fileHeader = *src.fileHeader;
	}
	if (infoHeader == NULL) {
		infoHeader = new BitmapInfoHeader;
	}
	*infoHeader = *src.infoHeader;

	unsigned paletteSize = getPaletteSize(infoHeader->bitCount);
	if (paletteSize > 0) {
		colorPalette = new RgbQuad[paletteSize];
		setPalette(src.colorPalette);
	}

	if (pixels == NULL) {
		isAllocated = true;
		pixels = new unsigned char[infoHeader->imageSize];
		memset(pixels, 255, infoHeader->imageSize);
	}
	bytesPerPixel = src.bytesPerPixel;
	rowBytes = src.rowBytes;
	rowPaddingBytes = src.rowPaddingBytes;
}

unsigned Dib::getPaletteSize(unsigned bitCount) {
	switch (bitCount) {
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
void Dib::readFromHandle(HANDLE handle)
{
	BITMAPINFOHEADER *dibHeaderPtr =
	(BITMAPINFOHEADER *) GlobalLock(handle);

	if (infoHeader != NULL) {
		delete infoHeader;
	}

	infoHeader = new BitmapInfoHeader;

	infoHeader->size = dibHeaderPtr->biSize;
	infoHeader->width = dibHeaderPtr->biWidth;
	infoHeader->height = dibHeaderPtr->biHeight;
	infoHeader->planes = dibHeaderPtr->biPlanes;
	infoHeader->bitCount = dibHeaderPtr->biBitCount;
	infoHeader->compression = dibHeaderPtr->biCompression;
	infoHeader->imageSize = dibHeaderPtr->biSizeImage;
	infoHeader->hPixelsPerMeter = dibHeaderPtr->biXPelsPerMeter;
	infoHeader->vPixelsPerMeter = dibHeaderPtr->biYPelsPerMeter;
	infoHeader->numColors = dibHeaderPtr->biClrUsed;
	infoHeader->numColorsImp = dibHeaderPtr->biClrImportant;

	unsigned paletteSize = getPaletteSize(infoHeader->bitCount);
	if (paletteSize > 0) {
		colorPalette =
		reinterpret_cast < RgbQuad * >(reinterpret_cast <
				unsigned char
				*>(dibHeaderPtr)
				+ sizeof(BITMAPINFOHEADER));
	}

	pixels =
	reinterpret_cast <
	unsigned char *>(dibHeaderPtr) + sizeof(BITMAPINFOHEADER)
	+ paletteSize * sizeof(RgbQuad);

	bytesPerPixel = infoHeader->bitCount >> 3;
	rowBytes = getRowBytes(infoHeader->width, infoHeader->bitCount);
	rowPaddingBytes = rowBytes - (infoHeader->width * bytesPerPixel);

	UA_DOUT(4, 5, "readFromHandle: "
			<< " size/" << infoHeader->size
			<< " width/" << infoHeader->width
			<< " height/" << infoHeader->height
			<< " bitCount/" << infoHeader->bitCount
			<< " imageSize/" << infoHeader->imageSize
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

	fileHeader = new BitmapFileHeader;
	infoHeader = new BitmapInfoHeader;
	memset(fileHeader, 0, sizeof(BitmapFileHeader));
	memset(infoHeader, 0, sizeof(BitmapInfoHeader));

	unsigned char fileHeaderRaw[0xE];
	unsigned char infoHeaderRaw[0x28];
	unsigned r;

	r = fread(fileHeaderRaw, sizeof(unsigned char), sizeof(fileHeaderRaw), fh);
	UA_ASSERT(r = sizeof(fileHeaderRaw));
	r = fread(infoHeaderRaw, sizeof(unsigned char), sizeof(infoHeaderRaw), fh);
	UA_ASSERT(r = sizeof(infoHeaderRaw));

	fileHeader->type = *(unsigned short *) &fileHeaderRaw[0];
	fileHeader->size = *(unsigned *) &fileHeaderRaw[2];
	fileHeader->reserved1 = *(unsigned short *) &fileHeaderRaw[6];
	fileHeader->reserved2 = *(unsigned short *) &fileHeaderRaw[8];
	fileHeader->offset = *(unsigned *) &fileHeaderRaw[0xA];

	infoHeader->size = *(unsigned *) &infoHeaderRaw[0];
	infoHeader->width = *(unsigned *) &infoHeaderRaw[0x12 - 0xE];
	infoHeader->height = *(unsigned *) &infoHeaderRaw[0x16 - 0xE];
	infoHeader->planes = *(unsigned short *) &infoHeaderRaw[0x1A - 0xE];
	infoHeader->bitCount = *(unsigned short *) &infoHeaderRaw[0x1C - 0xE];
	infoHeader->compression = *(unsigned *) &infoHeaderRaw[0x1E - 0xE];
	infoHeader->imageSize = *(unsigned *) &infoHeaderRaw[0x22 - 0xE];
	infoHeader->hPixelsPerMeter = *(unsigned *) &infoHeaderRaw[0x26 - 0xE];
	infoHeader->vPixelsPerMeter = *(unsigned *) &infoHeaderRaw[0x2A - 0xE];
	infoHeader->numColors = *(unsigned *) &infoHeaderRaw[0x2E - 0xE];
	infoHeader->numColorsImp = *(unsigned *) &infoHeaderRaw[0x32 - 0xE];

	unsigned paletteSize = getPaletteSize(infoHeader->bitCount);
	if (paletteSize > 0) {
		colorPalette = new RgbQuad[paletteSize];
		unsigned paletteBytes = paletteSize * sizeof(RgbQuad);
		r = fread(colorPalette, sizeof(unsigned char), paletteBytes, fh);
		UA_ASSERT(r == paletteBytes);
	}

	bytesPerPixel = infoHeader->bitCount >> 3;
	rowBytes = getRowBytes(infoHeader->width, infoHeader->bitCount);
	rowPaddingBytes = rowBytes - (infoHeader->width * bytesPerPixel);

	isAllocated = true;
	pixels = new unsigned char[infoHeader->imageSize];
	r = fread(pixels, sizeof(unsigned char), infoHeader->imageSize, fh);
	UA_ASSERT(r = infoHeader->imageSize);
	fclose(fh);

	UA_DOUT(4, 5, "readFromFile: rowBytes/" << rowBytes
			<< " paddingBytes/" << rowPaddingBytes);
}

unsigned Dib::getRowBytes(unsigned width, unsigned bitCount) {
	return static_cast<unsigned> (ceil((width * bitCount) / 32.0)) << 2;
}

unsigned Dib::getInternalRowBytes() {
	return rowBytes;
}

bool Dib::writeToFile(const char *filename) {
	UA_ASSERT_NOT_NULL(filename);
	UA_ASSERT_NOT_NULL(pixels);

	unsigned char fileHeaderRaw[0xE];
	unsigned char infoHeaderRaw[0x28];
	unsigned paletteSize = getPaletteSize(infoHeader->bitCount);
	unsigned paletteBytes = paletteSize * sizeof(RgbQuad);

	if (fileHeader != NULL) {
		*(unsigned short *) &fileHeaderRaw[0] = fileHeader->type;
		*(unsigned *) &fileHeaderRaw[2] = fileHeader->size;
		*(unsigned short *) &fileHeaderRaw[6] = fileHeader->reserved1;
		*(unsigned short *) &fileHeaderRaw[8] = fileHeader->reserved2;
		*(unsigned *) &fileHeaderRaw[0xA] = fileHeader->offset;
	} else {
		*(unsigned short *) &fileHeaderRaw[0] = 0x4D42;
		*(unsigned *) &fileHeaderRaw[2] = infoHeader->imageSize
				+ sizeof(fileHeaderRaw) + sizeof(infoHeaderRaw) + paletteBytes;
		*(unsigned short *) &fileHeaderRaw[6] = 0;
		*(unsigned short *) &fileHeaderRaw[8] = 0;
		*(unsigned *) &fileHeaderRaw[0xA] = sizeof(fileHeaderRaw)
				+ sizeof(infoHeaderRaw) + paletteBytes;
	}

	*(unsigned *) &infoHeaderRaw[0] = infoHeader->size;
	*(unsigned *) &infoHeaderRaw[0x12 - 0xE] = infoHeader->width;
	*(unsigned *) &infoHeaderRaw[0x16 - 0xE] = infoHeader->height;
	*(unsigned short *) &infoHeaderRaw[0x1A - 0xE] = infoHeader->planes;
	*(unsigned short *) &infoHeaderRaw[0x1C - 0xE] = infoHeader->bitCount;
	*(unsigned *) &infoHeaderRaw[0x1E - 0xE] = infoHeader->compression;
	*(unsigned *) &infoHeaderRaw[0x22 - 0xE] = infoHeader->imageSize;
	*(unsigned *) &infoHeaderRaw[0x26 - 0xE] = infoHeader->hPixelsPerMeter;
	*(unsigned *) &infoHeaderRaw[0x2A - 0xE] = infoHeader->vPixelsPerMeter;
	*(unsigned *) &infoHeaderRaw[0x2E - 0xE] = infoHeader->numColors;
	*(unsigned *) &infoHeaderRaw[0x32 - 0xE] = infoHeader->numColorsImp;

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
		r = fwrite(colorPalette, sizeof(unsigned char), paletteBytes, fh);
		UA_ASSERT(r == paletteBytes);
	}
	r = fwrite(pixels, sizeof(unsigned char), infoHeader->imageSize, fh);
	UA_ASSERT(r == infoHeader->imageSize);
	fclose(fh);
	return true;
}

unsigned Dib::getHeight() {
	return infoHeader->height;
}

unsigned Dib::getWidth() {
	return infoHeader->width;
}

unsigned Dib::getRowPadBytes() {
	return rowPaddingBytes;
}

unsigned Dib::getBitsPerPixel() {
	return infoHeader->bitCount;
}

unsigned char *Dib::getPixelBuffer() {
	UA_ASSERT_NOT_NULL(pixels);
	return pixels;
}

unsigned char *Dib::getRowPtr(unsigned row) {
	UA_ASSERT(row < infoHeader->height);
	return pixels + row * rowBytes;
}

void Dib::getPixel(unsigned row, unsigned col, RgbQuad & quad) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned char *ptr = pixels + row * rowBytes + col * bytesPerPixel;

	assert(infoHeader->bitCount != 8);
	quad.rgbRed = ptr[0];
	quad.rgbGreen = ptr[1];
	quad.rgbBlue = ptr[2];
	quad.rgbReserved = 0;
}

unsigned char Dib::getPixelAvgGrayscale(unsigned row, unsigned col) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned char *ptr = pixels + row * rowBytes + col * bytesPerPixel;

	if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
		return (unsigned char) (0.33333 * ptr[0] + 0.33333 * ptr[1] + 0.33333
				* ptr[2]);
	} else if (infoHeader->bitCount == 8) {
		return *ptr;
	}
	UA_ASSERTS(false,
			"bitCount " << infoHeader->bitCount <<
			" not implemented yet");
	return 0;
}

inline unsigned char Dib::getPixelGrayscale(unsigned row, unsigned col) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned char *ptr = pixels + row * rowBytes + col * bytesPerPixel;

	if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
		return static_cast<unsigned char> (0.3333 * ptr[0] + 0.3333 * ptr[1]
				+ 0.3333 * ptr[2]);
	} else if (infoHeader->bitCount == 8) {
		return *ptr;
	}
	UA_ASSERTS(false,
			"bitCount " << infoHeader->bitCount <<
			" not implemented yet");
	return 0;
}

void Dib::setPixel(unsigned x, unsigned y, RgbQuad & quad) {
	UA_ASSERT(x < infoHeader->width);
	UA_ASSERT(y < infoHeader->height);

	unsigned char *ptr = (pixels + y * rowBytes + x * bytesPerPixel);

	if (infoHeader->bitCount == 8) {
		*ptr = static_cast<unsigned char> (0.3333 * quad.rgbRed + 0.3333
				* quad.rgbGreen + 0.3333 * quad.rgbBlue);
		return;
	}

	if ((infoHeader->bitCount != 24) && (infoHeader->bitCount != 32)) {
		UA_ERROR("can't assign RgbQuad to dib");
	}

	ptr[2] = quad.rgbRed;
	ptr[1] = quad.rgbGreen;
	ptr[0] = quad.rgbBlue;
}

inline void Dib::setPixelGrayscale(unsigned row, unsigned col,
		unsigned char value) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned char *ptr = pixels + row * rowBytes + col * bytesPerPixel;

	if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
		ptr[0] = value;
		ptr[1] = value;
		ptr[2] = value;
	} else if (infoHeader->bitCount == 8) {
		*ptr = value;
	} else {
		assert(0); /* can't assign RgbQuad to dib */
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
auto_ptr<Dib> Dib::crop(Dib & src, unsigned x0, unsigned y0, unsigned x1, unsigned y1) {
	UA_ASSERT_NOT_NULL(src.infoHeader);
	UA_ASSERT(x1 > x0);
	UA_ASSERT(y1 > y0);

	bound(0, x0, src.infoHeader->width);
	bound(0, x1, src.infoHeader->width);
	bound(0, y0, src.infoHeader->height);
	bound(0, y1, src.infoHeader->height);

    unsigned width = x1 - x0;
    unsigned height = y1 - y0;

    auto_ptr<Dib> dest(new Dib(width, height, src.infoHeader->bitCount));

    dest->infoHeader->hPixelsPerMeter = src.infoHeader->hPixelsPerMeter;
    dest->infoHeader->vPixelsPerMeter = src.infoHeader->vPixelsPerMeter;

	unsigned char *srcRowPtr = src.pixels + (src.infoHeader->height - y1)
			* src.rowBytes + x0 * dest->bytesPerPixel;
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
		return auto_ptr<Dib> (&src);
	}

	UA_DOUT(4, 9, "convertGrayscale: Converting from 24 bit to 8 bit.");

	// 24bpp -> 8bpp
	auto_ptr<Dib>
			dest(new Dib(src.infoHeader->width, src.infoHeader->height, 8));

	dest->infoHeader->hPixelsPerMeter = src.infoHeader->hPixelsPerMeter;
	dest->infoHeader->vPixelsPerMeter = src.infoHeader->vPixelsPerMeter;

	UA_DOUT(4, 9, "convertGrayscale: Made dib");

	unsigned char *srcRowPtr = src.pixels;
	unsigned char *destRowPtr = dest->pixels;
	unsigned char *srcPtr, *destPtr;

	for (unsigned row = 0; row < src.infoHeader->height; ++row) {
		srcPtr = srcRowPtr;
		destPtr = destRowPtr;
		for (unsigned col = 0; col < src.infoHeader->width; ++col, ++destPtr) {
			*destPtr = static_cast<unsigned char> (0.3333 * srcPtr[0] + 0.3333
					* srcPtr[1] + 0.3333 * srcPtr[2]);
			srcPtr += src.bytesPerPixel;
		}

		// now initialise the padding bytes
		destPtr = destRowPtr + dest->infoHeader->width;
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
	UA_ASSERTS(infoHeader != NULL, "NULL infoHeader specified to generateIplImage");
	UA_ASSERTS(infoHeader->bitCount == 8, "generateIplImage requires an unsigned 8bit image");
	UA_ASSERTS(pixels != NULL,"NULL pixel data specified to generateIplImage");

	IplImage *image = NULL;
	CvMat hdr, *matrix = NULL;
	CvSize size;

	size.width = infoHeader->width;
	size.height = infoHeader->height;

	image = cvCreateImage(size, IPL_DEPTH_8U, 1);
	matrix = cvGetMat(image, &hdr);

	memcpy(matrix->data.ptr, pixels, infoHeader->imageSize);

	cvFlip(image, image, 0);

    auto_ptr<IplImageContainer> iplContainer(new IplImageContainer(NULL));
	iplContainer->setIplImage(image);
	iplContainer->setHorizontalResolution(infoHeader->hPixelsPerMeter);
	iplContainer->setVerticalResolution(infoHeader->vPixelsPerMeter);
	return iplContainer;
}

void Dib::rectangle(unsigned x, unsigned y, unsigned width, unsigned height,
		RgbQuad & quad) {
	line(x, y, x, y + height, quad);
	line(x, y, x + width, y, quad);
	line(x + width, y, x + width, y + height, quad);
	line(x, y + height, x + width, y + height, quad);

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
	UA_ASSERT_NOT_NULL(infoHeader);
	UA_ASSERT(y0 < infoHeader->height);
	UA_ASSERT(y1 < infoHeader->height);
	UA_ASSERT(x0 < infoHeader->width);
	UA_ASSERT(x1 < infoHeader->width);

	unsigned x, deltax, y, deltay, st;
	int cx, cy, error, xstep, ystep;

	y0 = infoHeader->height - y0 - 1;
	y1 = infoHeader->height - y1 - 1;

	// find largest delta for pixel steps
	deltax = abs(static_cast<int> (x1) - static_cast<int> (x0));
	deltay = abs(static_cast<int> (y1) - static_cast<int> (y0));

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

	deltax = abs(static_cast<int> (x1) - static_cast<int> (x0));
	deltay = abs(static_cast<int> (y1) - static_cast<int> (y0));
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
		UA_DOUT(4, 5, "tpPresetFilter: No filter applied (default) dpi/" << getDpi());
		break;
	}
}

// Can only be used for grayscale Dibs
void Dib::convolveFast3x3(const float(&k)[9]) {
	UA_ASSERTS(infoHeader->bitCount == 8,
			"convolveFast3x3 requires an unsigned 8bit image");

	int nc = infoHeader->width;
	int nr = infoHeader->height;
	unsigned size = nr * nc;

	float *imageOut = new float[size];
	float *imageIn = new float[size];

	fill(imageOut, imageOut + size, 0.0f);
	{
		unsigned char *srcRowPtr = pixels, *srcPtr;
		float *destPtr = imageIn;

		for (unsigned row = 0; row < infoHeader->height; row++) {
			srcPtr = srcRowPtr;
			for (unsigned col = 0; col < infoHeader->width; col++, srcPtr++, destPtr++) {
				*destPtr = *srcPtr;
			}
			srcRowPtr += rowBytes;
		}
	}

	int ncm1 = nc - 1, nrm1 = nr - 1;
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
		float *r00 = imageIn + (i - 1) * nc;
		float *r01 = r00 + 1;
		float *r02 = r01 + 1;
		float *r10 = r00 + nc;
		float *r11 = r10 + 1;
		float *r12 = r11 + 1;
		float *r20 = r10 + nc;
		float *r21 = r20 + 1;
		float *r22 = r21 + 1;
		float *rOut = imageOut + i * nc + 1;
		for (int j = 1; j < ncm1; j++) {
			*rOut++ = (k00 * *r00++) + (k01 * *r01++) + (k02 * *r02++) + (k10
					* *r10++) + (k11 * *r11++) + (k12 * *r12++)
					+ (k20 * *r20++) + (k21 * *r21++) + (k22 * *r22++);
		}
	}

	{
		float *srcPtr = imageOut;
		unsigned char *destRowPtr = pixels, *destPtr;

		for (unsigned row = 0; row < infoHeader->height; row++) {
			destPtr = destRowPtr;
			for (unsigned col = 0; col < infoHeader->width; col++, srcPtr++, destPtr++) {
				*destPtr = static_cast<unsigned char> (*srcPtr);
			}
			destRowPtr += rowBytes;
		}
	}

	delete[] imageIn;
	delete[] imageOut;
}

unsigned Dib::getDpi() {
	// 1 inch = 0.0254 meters
	return static_cast<unsigned> (infoHeader->hPixelsPerMeter * 0.0254 + 0.5);
}

