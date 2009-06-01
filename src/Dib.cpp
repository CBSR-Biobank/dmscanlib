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
	fileHeader(NULL), infoHeader(NULL), pixels(NULL),
	isAllocated(false) {
}

Dib::Dib(char * filename) :
	fileHeader(NULL), infoHeader(NULL), pixels(NULL),
	isAllocated(false) {
	readFromFile(filename);
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

Dib::~Dib() {
	if (fileHeader != NULL) {
		delete fileHeader;
	}
	delete infoHeader;

	if ((isAllocated) && (pixels != NULL)) {
		delete pixels;
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
	unsigned rowBytes = infoHeader->width * bytesPerPixel + rowPaddingBytes;
	return pixels + row * rowBytes;
}

void Dib::getPixel(unsigned row, unsigned col, RgbQuad * quad) {
	unsigned rowBytes = infoHeader->width * bytesPerPixel + rowPaddingBytes;
	unsigned char * ptr = pixels + row * rowBytes + col * bytesPerPixel;

	assert(infoHeader->bitCount != 8);
	quad->rgbRed      = ptr[0];
	quad->rgbGreen    = ptr[1];
	quad->rgbBlue     = ptr[2];
	quad->rgbReserved = 0;
}

unsigned char Dib::getPixelGrayscale(unsigned row, unsigned col) {
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

void Dib::setPixel(unsigned row, unsigned col, RgbQuad * quad) {
	unsigned padding = (infoHeader->width * bytesPerPixel) & 0x3;
	unsigned rowBytes = infoHeader->width * bytesPerPixel + padding;
	unsigned char * ptr = (pixels + row * rowBytes + col * bytesPerPixel);

	if ((infoHeader->bitCount != 24) && (infoHeader->bitCount != 32)) {
		UA_ERROR("can't assign RgbQuad to dib");
	}
	ptr[0] = quad->rgbRed;
	ptr[1] = quad->rgbGreen;
	ptr[2] = quad->rgbBlue;
}

void Dib::setPixelGrayscale(unsigned row, unsigned col,	unsigned char value) {
	unsigned padding = (infoHeader->width * bytesPerPixel) & 0x3;
	unsigned rowBytes = infoHeader->width * bytesPerPixel + padding;
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

	int I, J;
	unsigned Y, X, SUM;
	long sum1, sum2;
	unsigned width = src.infoHeader->width;
	unsigned height = src.infoHeader->height;

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
						sum1 += src.getPixelGrayscale(Y + J, X + I)
						* mask1[I+1][J+1];
					}
				}

				if (mask2 != NULL) {
					for (I = -1; I <= 1; ++I)  {
						for (J = -1; J <= 1; ++J)  {
							sum2 += src.getPixelGrayscale(Y + J, X + I)
							* mask2[I+1][J+1];
						}
					}
				}

				/*---GRADIENT MAGNITUDE APPROXIMATION (Myler p.218)----*/
				SUM = abs(sum1) + abs(sum2);
			}

			if (SUM > 255) SUM = 255;
			if (SUM < 0) SUM = 0;

			setPixelGrayscale(Y, X, (unsigned char)(255 - SUM));
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
