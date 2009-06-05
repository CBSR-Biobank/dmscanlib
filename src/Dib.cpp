/*******************************************************************************
 * Device Independent Bitmap
 ******************************************************************************/

#include "Dib.h"
#include "UaDebug.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#else
#include <strings.h>
#endif

Dib::Dib() :
	fileHeader(NULL), infoHeader(NULL), pixels(NULL), isAllocated(false) {
}

Dib::Dib(Dib & src) :
	fileHeader(NULL), infoHeader(NULL), pixels(NULL), isAllocated(false) {
	copyInternals(src);
	memcpy(pixels, src.pixels, infoHeader->imageSize);
}

Dib::Dib(unsigned rows, unsigned cols, unsigned colorBits)  :
	fileHeader(NULL) {
	bytesPerPixel = colorBits >> 3;
	rowPaddingBytes = (cols * bytesPerPixel) & 0x3;

	unsigned rowBytes = cols * bytesPerPixel + rowPaddingBytes;

	infoHeader = new BitmapInfoHeader;
	infoHeader->size            = 40;
	infoHeader->width           = cols;
	infoHeader->height          = rows;
	infoHeader->planes          = 1;
	infoHeader->bitCount        = colorBits;
	infoHeader->compression     = 0;
	infoHeader->imageSize       = infoHeader->height * rowBytes;
	infoHeader->hPixelsPerMeter = 0;
	infoHeader->vPixelsPerMeter = 0;
	infoHeader->numColors       = 0;
	infoHeader->numColorsImp    = 0;

	isAllocated = true;
	pixels = new unsigned char[infoHeader->imageSize];
	memset(pixels, 255, infoHeader->imageSize);
}

Dib::Dib(char * filename) :
	fileHeader(NULL), infoHeader(NULL), pixels(NULL),
	isAllocated(false) {
	readFromFile(filename);
}

Dib::~Dib() {
	if (fileHeader != NULL) {
		delete fileHeader;
	}
	delete infoHeader;

	if ((isAllocated) && (pixels != NULL)) {
		delete [] pixels;
	}
}

void Dib::copyInternals(Dib & src) {
	if ((fileHeader != NULL) && (src.fileHeader != NULL)) {
		*fileHeader = *src.fileHeader;
	}
	if (infoHeader == NULL) {
		infoHeader = new BitmapInfoHeader;
	}
	*infoHeader = *src.infoHeader;
	if (pixels == NULL) {
		isAllocated = true;
		pixels = new unsigned char[infoHeader->imageSize];
	}

	bytesPerPixel = src.bytesPerPixel;
	rowPaddingBytes = src.rowPaddingBytes;
}

#ifdef WIN32
void Dib::readFromHandle(HANDLE handle) {
	BITMAPINFOHEADER * dibHeaderPtr = (BITMAPINFOHEADER *) GlobalLock(handle);

	infoHeader = new BitmapInfoHeader;

	infoHeader->size            = dibHeaderPtr->biSize;
	infoHeader->width           = dibHeaderPtr->biWidth;
	infoHeader->height          = dibHeaderPtr->biHeight;
	infoHeader->planes          = dibHeaderPtr->biPlanes;
	infoHeader->bitCount        = dibHeaderPtr->biBitCount;
	infoHeader->compression     = dibHeaderPtr->biCompression;
	infoHeader->imageSize       = dibHeaderPtr->biSizeImage;
	infoHeader->hPixelsPerMeter = dibHeaderPtr->biXPelsPerMeter;
	infoHeader->vPixelsPerMeter = dibHeaderPtr->biYPelsPerMeter;
	infoHeader->numColors       = dibHeaderPtr->biClrUsed;
	infoHeader->numColorsImp    = dibHeaderPtr->biClrImportant;

	pixels = (unsigned char *) dibHeaderPtr + sizeof(BITMAPINFOHEADER);

	bytesPerPixel = infoHeader->bitCount >> 3;
	rowPaddingBytes = (infoHeader->width * bytesPerPixel) & 0x3;
}
#endif

/**
 * All values in little-endian except for BitmapFileHeader.type.
 */
void Dib::readFromFile(const char * filename) {
	UA_ASSERT_NOT_NULL(filename);

	FILE * fh = fopen(filename, "r"); // C4996
	if (fh == NULL) {
		UA_ERROR("could not open file " << filename);
	}

	fileHeader = new BitmapFileHeader;
	infoHeader = new BitmapInfoHeader;
	memset(fileHeader, 0, sizeof(BitmapFileHeader));
	memset(infoHeader, 0, sizeof(BitmapInfoHeader));

	unsigned char fileHeaderRaw[0xE];
	unsigned char infoHeaderRaw[0x28];

	fread(fileHeaderRaw, 1, sizeof(fileHeaderRaw), fh);
	fread(infoHeaderRaw, 1, sizeof(infoHeaderRaw), fh);

	fileHeader->type      = *(unsigned short *)&fileHeaderRaw[0];
	fileHeader->size      = *(unsigned *)&fileHeaderRaw[2];
	fileHeader->reserved1 = *(unsigned short *)&fileHeaderRaw[6];
	fileHeader->reserved2 = *(unsigned short *)&fileHeaderRaw[8];
	fileHeader->offset    = *(unsigned *)&fileHeaderRaw[0xA];

	infoHeader->size            = *(unsigned *)&infoHeaderRaw[0];
	infoHeader->width           = *(unsigned *)&infoHeaderRaw[0x12 - 0xE];
	infoHeader->height          = *(unsigned *)&infoHeaderRaw[0x16 - 0xE];
	infoHeader->planes          = *(unsigned short *)&infoHeaderRaw[0x1A - 0xE];
	infoHeader->bitCount        = *(unsigned short *)&infoHeaderRaw[0x1C - 0xE];
	infoHeader->compression     = *(unsigned *)&infoHeaderRaw[0x1E - 0xE];
	infoHeader->imageSize       = *(unsigned *)&infoHeaderRaw[0x22 - 0xE];
	infoHeader->hPixelsPerMeter = *(unsigned *)&infoHeaderRaw[0x26 - 0xE];
	infoHeader->vPixelsPerMeter = *(unsigned *)&infoHeaderRaw[0x2A - 0xE];
	infoHeader->numColors       = *(unsigned *)&infoHeaderRaw[0x2E - 0xE];
	infoHeader->numColorsImp    = *(unsigned *)&infoHeaderRaw[0x32 - 0xE];

	bytesPerPixel = infoHeader->bitCount >> 3;
	rowPaddingBytes = (infoHeader->width * bytesPerPixel) & 0x3;

	isAllocated = true;
	pixels = new unsigned char[infoHeader->imageSize];
	fread(pixels, 1, infoHeader->imageSize, fh);
	fclose(fh);
}

void Dib::writeToFile(const char * filename) {
	UA_ASSERT_NOT_NULL(filename);
	UA_ASSERT_NOT_NULL(pixels);

	unsigned char fileHeaderRaw[0xE];
	unsigned char infoHeaderRaw[0x28];

	if (fileHeader != NULL) {
		*(unsigned short *)&fileHeaderRaw[0] = fileHeader->type;
		*(unsigned *)&fileHeaderRaw[2]       = fileHeader->size;
		*(unsigned short *)&fileHeaderRaw[6] = fileHeader->reserved1;
		*(unsigned short *)&fileHeaderRaw[8] = fileHeader->reserved2;
		*(unsigned *)&fileHeaderRaw[0xA]     = fileHeader->offset;
	}
	else {
		*(unsigned short *)&fileHeaderRaw[0] = 0x4D42;
		*(unsigned *)&fileHeaderRaw[2]       =
			infoHeader->imageSize + sizeof(fileHeaderRaw) + sizeof(infoHeaderRaw);
		*(unsigned short *)&fileHeaderRaw[6] = 0;
		*(unsigned short *)&fileHeaderRaw[8] = 0;
		*(unsigned *)&fileHeaderRaw[0xA]     = 54;
	}

	*(unsigned *)&infoHeaderRaw[0]                = infoHeader->size;
	*(unsigned *)&infoHeaderRaw[0x12 - 0xE]       = infoHeader->width;
	*(unsigned *)&infoHeaderRaw[0x16 - 0xE]       = infoHeader->height;
	*(unsigned short *)&infoHeaderRaw[0x1A - 0xE] = infoHeader->planes;
	*(unsigned short *)&infoHeaderRaw[0x1C - 0xE] = infoHeader->bitCount;
	*(unsigned *)&infoHeaderRaw[0x1E - 0xE]       = infoHeader->compression;
	*(unsigned *)&infoHeaderRaw[0x22 - 0xE]       = infoHeader->imageSize;
	*(unsigned *)&infoHeaderRaw[0x26 - 0xE]       = infoHeader->hPixelsPerMeter;
	*(unsigned *)&infoHeaderRaw[0x2A - 0xE]       = infoHeader->vPixelsPerMeter;
	*(unsigned *)&infoHeaderRaw[0x2E - 0xE]       = infoHeader->numColors;
	*(unsigned *)&infoHeaderRaw[0x32 - 0xE]       = infoHeader->numColorsImp;

	FILE * fh = fopen(filename, "wb"); // C4996
	UA_ASSERTS(fh != NULL,
		"could not open file for writing" << filename);

	fwrite(fileHeaderRaw, 1, sizeof(fileHeaderRaw), fh);
	fwrite(infoHeaderRaw, 1, sizeof(infoHeaderRaw), fh);
	fwrite(pixels, 1, infoHeader->imageSize, fh);
	fclose(fh);
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

unsigned char * Dib::getPixelBuffer() {
	UA_ASSERT_NOT_NULL(pixels);
	return pixels;
}

unsigned char * Dib::getRowPtr(unsigned row) {
	UA_ASSERT(row < infoHeader->height);

	unsigned rowBytes = infoHeader->width * bytesPerPixel + rowPaddingBytes;
	return pixels + row * rowBytes;
}

void Dib::getPixel(unsigned row, unsigned col, RgbQuad & quad) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned rowBytes = infoHeader->width * bytesPerPixel + rowPaddingBytes;
	unsigned char * ptr = pixels + row * rowBytes + col * bytesPerPixel;

	assert(infoHeader->bitCount != 8);
	quad.rgbRed      = ptr[0];
	quad.rgbGreen    = ptr[1];
	quad.rgbBlue     = ptr[2];
	quad.rgbReserved = 0;
}

unsigned char Dib::getPixelGrayscale(unsigned row, unsigned col) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned rowBytes = infoHeader->width * bytesPerPixel + rowPaddingBytes;
	unsigned char * ptr = pixels + row * rowBytes + col * bytesPerPixel;

	if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
		return (unsigned char) (0.3 * ptr[0] + 0.59 * ptr[1] + 0.11 * ptr[2]);
	}
	else if (infoHeader->bitCount == 8) {
		return *ptr;
	}
	UA_ASSERTS(false, "bitCount " << infoHeader->bitCount << " not implemented yet");
	return  0;
}

void Dib::setPixel(unsigned row, unsigned col, RgbQuad & quad) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned rowBytes = infoHeader->width * bytesPerPixel + rowPaddingBytes;
	unsigned char * ptr = (pixels + row * rowBytes + col * bytesPerPixel);

	if ((infoHeader->bitCount != 24) && (infoHeader->bitCount != 32)) {
		UA_ERROR("can't assign RgbQuad to dib");
	}
	ptr[2] = quad.rgbRed;
	ptr[1] = quad.rgbGreen;
	ptr[0] = quad.rgbBlue;
}

void Dib::setPixelGrayscale(unsigned row, unsigned col,	unsigned char value) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned rowBytes = infoHeader->width * bytesPerPixel + rowPaddingBytes;
	unsigned char * ptr = pixels + row * rowBytes + col * bytesPerPixel;

	if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
		ptr[0] = value;
		ptr[1] = value;
		ptr[2] = value;
	}
	else if (infoHeader->bitCount == 8) {
		*ptr = value;
	}
	else {
		assert(0); /* can't assign RgbQuad to dib */
	}
}

/*
 * DIBs are flipped in Y;
 */
void Dib::crop(Dib &src, unsigned x0, unsigned y0, unsigned x1, unsigned y1) {
	UA_ASSERT_NOT_NULL(src.infoHeader);
	UA_ASSERT(x1 > x0);
	UA_ASSERT(y1 > y0);

	infoHeader = new BitmapInfoHeader;
	*infoHeader = *src.infoHeader;
	bytesPerPixel = src.bytesPerPixel;

	UA_ASSERT(y0 < infoHeader->height);
	UA_ASSERT(y1 < infoHeader->height);
	UA_ASSERT(x0 < infoHeader->width);
	UA_ASSERT(x1 < infoHeader->width);

	infoHeader->width  = x1 - x0;
	infoHeader->height = y1 - y0;

	rowPaddingBytes = (infoHeader->width * bytesPerPixel) & 0x3;
	unsigned destRowBytes = infoHeader->width * bytesPerPixel + rowPaddingBytes;

	infoHeader->imageSize = infoHeader->height * destRowBytes;

	isAllocated = true;
	pixels = new unsigned char[infoHeader->imageSize];

	unsigned srcRowBytes = src.infoHeader->width * src.bytesPerPixel + src.rowPaddingBytes;
	unsigned char * srcRowPtr = src.pixels + (src.infoHeader->height - y1) * srcRowBytes + x0 * bytesPerPixel;
	unsigned char * destRowPtr = pixels;

	for (unsigned row = 0; row < infoHeader->height;
		++row, srcRowPtr += srcRowBytes, destRowPtr += destRowBytes) {
		memcpy(destRowPtr, srcRowPtr, infoHeader->width * bytesPerPixel);
	}
}

void Dib::convertGrayscale(Dib & src) {
	UA_ASSERT_NOT_NULL(src.infoHeader);

	copyInternals(src);
	for (unsigned row = 0; row < src.infoHeader->height; ++row) {
		for (unsigned col = 0; col < src.infoHeader->width; ++col) {
			setPixelGrayscale(row, col,	src.getPixelGrayscale(row, col));
		}
	}
}

void Dib::sobelEdgeDetectionWithMask(Dib & src, int mask1[3][3],
		int mask2[3][3]) {
	UA_ASSERT_NOT_NULL(src.infoHeader);

	unsigned rowBytes = infoHeader->width * bytesPerPixel + rowPaddingBytes;

	int I, J;
	unsigned Y, X, SUM;
	long sum1, sum2;
	unsigned width = src.infoHeader->width;
	unsigned height = src.infoHeader->height;

	unsigned char * srcRowPtr = src.pixels, * destRowPtr = pixels;
	unsigned char * pixel, * pixelStart, pixelValue;

	assert(mask1 != NULL);

	for (Y = 0; Y <= height - 1; ++Y, srcRowPtr += rowBytes, destRowPtr += rowBytes)  {
		pixelStart = srcRowPtr;
		for (X = 0; X <= width - 1 ; ++X, pixelStart += bytesPerPixel)  {
			sum1 = 0;
			sum2 = 0;

			/* image boundaries */
			if ((Y == 0) || (Y == height - 1)
					|| (X == 0) || (X == width - 1)) {
				SUM = 0;
			}
			else {
				/* Convolution starts here */
				for (J = -1; J <= 1; ++J, pixel += rowBytes)  {
					pixel = pixelStart;
					for (I = -1; I <= 1; ++I, pixel += bytesPerPixel)  {
						if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
							// convert to grayscale
							pixelValue = (unsigned char)(
									0.3 * pixel[0] + 0.59 * pixel[1] + + 0.11 * pixel[2]);
						}
						else {
							pixelValue = pixel[0];
						}

						sum1 += pixelValue * mask1[I+1][J+1];
						if (mask2 != NULL) {
							sum2 += pixelValue * mask2[I+1][J+1];

						}
					}
				}

				/*---GRADIENT MAGNITUDE APPROXIMATION (Myler p.218)----*/
				SUM = abs(sum1) + abs(sum2);
			}

			if (SUM > 255) SUM = 255;
			if (SUM < 0) SUM = 0;

			pixel = destRowPtr + X * bytesPerPixel;
			pixel[0] = (unsigned char)(255 - SUM);
			if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
				pixel[1] = pixel[2] = pixel[0];
			}
		}
	}
}

/**
 * Taken from: http://www.pages.drexel.edu/~weg22/edge.html
 */
void Dib::sobelEdgeDetection(Dib & src) {
	UA_ASSERT_NOT_NULL(src.infoHeader);

	int GX[3][3], GY[3][3];

	/* 3x3 GX Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html */
	GX[0][0] = -1; GX[0][1] = 0; GX[0][2] = 1;
	GX[1][0] = -2; GX[1][1] = 0; GX[1][2] = 2;
	GX[2][0] = -1; GX[2][1] = 0; GX[2][2] = 1;

	/* 3x3 GY Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html */
	GY[0][0] =  1; GY[0][1] =  2; GY[0][2] =  1;
	GY[1][0] =  0; GY[1][1] =  0; GY[1][2] =  0;
	GY[2][0] = -1; GY[2][1] = -2; GY[2][2] = -1;

	copyInternals(src);
	sobelEdgeDetectionWithMask(src, GY, NULL); //GX, GY);
}

/**
 * Taken from: http://www.pages.drexel.edu/~weg22/edge.html
 */
void Dib::laplaceEdgeDetection(Dib & src) {
	UA_ASSERT_NOT_NULL(src.infoHeader);

	unsigned width = src.infoHeader->width;
	unsigned height = src.infoHeader->height;

	unsigned Y, X, SUM;
	int I, J, MASK[5][5];

	copyInternals(src);

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
						SUM = SUM + src.getPixelGrayscale(Y + J, X + I)
						* MASK[I+2][J+2];
					}
				}
			}

			if (SUM > 255)  SUM = 255;
			if (SUM < 0)    SUM = 0;

			setPixelGrayscale(Y, X, (unsigned char)(255 - SUM));
		}
	}
}

void Dib::histEqualization(Dib & src) {
	UA_ASSERT_NOT_NULL(src.infoHeader);

	unsigned width = src.infoHeader->width;
	unsigned height = src.infoHeader->height;

	unsigned imgSize = width * height;
	unsigned histogram[256];
	double sum[256], runningSum;
	unsigned row, col, i;

	copyInternals(src);

	memset(&histogram, 0, sizeof(histogram));
	memset(&sum, 0, sizeof(sum));

	for (row = 0; row < height; ++row) {
		for (col = 0; col < width; ++col) {
			histogram[src.getPixelGrayscale(row, col)]++;
		}
	}

	for (i = 0, runningSum = 0; i <= 255; ++i) {
		runningSum += (histogram[i] / (double) imgSize);
		sum[i] = runningSum;
	}

	for (row = 0; row < height; ++row) {
		for (col = 0; col < width; ++col) {
			setPixelGrayscale(
					row, col,
					(unsigned char) (256 * sum[src.getPixelGrayscale(row, col)]));
		}
	}
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
void Dib::line(unsigned x0, unsigned y0, unsigned x1, unsigned y1, RgbQuad & quad) {
	UA_ASSERT_NOT_NULL(infoHeader);
	UA_ASSERT(y0 < infoHeader->height);
	UA_ASSERT(y1 < infoHeader->height);
	UA_ASSERT(x0 < infoHeader->width);
	UA_ASSERT(x1 < infoHeader->width);

	unsigned x, deltax, y, deltay, st;
	int cx, cy, error, xstep, ystep;

	y0 =  infoHeader->height - y0;
	y1 =  infoHeader->height - y1;

	// find largest delta for pixel steps
	st = (abs(y1 - y0) > abs(x1 - x0));

	// if deltay > deltax then swap x,y
	if (st) {
		x0 ^= y0; y0 ^= x0; x0 ^= y0; // swap(x0, y0);
		x1 ^= y1; y1 ^= x1; x1 ^= y1; // swap(x1, y1);
	}

	deltax = abs(x1 - x0);
	deltay = abs(y1 - y0);
	error  = (deltax / 2);
	y = y0;

	if (x0 > x1) { xstep = -1; }
	else         { xstep =  1; }

	if (y0 > y1) { ystep = -1; }
	else         { ystep =  1; }

	for (x = x0; x != x1 + xstep; x += xstep) {
		cx = x; cy = y; // copy of x, copy of y

		// if x,y swapped above, swap them back now
		if (st) {
			cx ^= cy; cy ^= cx; cx ^= cy;
		}

		setPixel(cy, cx, quad);
		error -= deltay; // converge toward end of line

		if (error < 0) { // not done yet
			y += ystep;
			error += deltax;
		}
	}
}
