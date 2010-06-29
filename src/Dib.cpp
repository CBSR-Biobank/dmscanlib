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
const float Dib::BLUR_KERNEL[9] = { 0.0f, 0.2f, 0.0f,
									0.2f, 0.2f, 0.2f,
									0.0f, 0.2f, 0.0f, };
// expects sharp image
const float Dib::BLANK_KERNEL[9] = {
				0.06185567f, 0.12371134f,  0.06185567f,
				0.12371134f, 0.257731959f, 0.12371134f,
				0.06185567f, 0.12371134f,  0.06185567f, };

// performs poorly on black 2d barcodes on white
const float
		Dib::DPI_400_KERNEL[9] = {
			0.0587031f, 0.1222315f, 0.0587031f,
			0.1222315f, 0.2762618f, 0.1222315f,
			0.0587031f, 0.1222315f, 0.0587031f, };

Dib::Dib() :
	fileHeader(NULL), infoHeader(NULL), colorPalette(NULL), pixels(NULL),
			isAllocated(false) {
	ua::Logger::Instance().subSysHeaderSet(4, "Dib");
}

Dib::Dib(Dib & src) :
	fileHeader(NULL), infoHeader(NULL), colorPalette(NULL), pixels(NULL),
			isAllocated(false) {
	ua::Logger::Instance().subSysHeaderSet(4, "Dib");
	copyInternals(src);
	memcpy(pixels, src.pixels, infoHeader->imageSize);
}

Dib::Dib(IplImageContainer & img){

	IplImage * src = img.getIplImage();

	UA_ASSERTS(src->depth == IPL_DEPTH_8U, 
		"Dib::Dib(IplImage & src) requires an unsigned 8bit image");

	CvMat hdr, *matrix = NULL;
	/* IplImage saves a flipped image*/
	cvFlip(src,src,0);
	matrix = cvGetMat( src, &hdr );

	bytesPerPixel = 1;
	rowBytes = src->widthStep;
	rowPaddingBytes = src->widthStep - (src->width * 1);

	fileHeader = NULL;
	infoHeader = new BitmapInfoHeader;

	infoHeader->size = 40; /* bytes in header should be 40*/
	infoHeader->width = src->width;
	infoHeader->height = src->height;
	infoHeader->planes = 1;
	infoHeader->bitCount = 8;
	infoHeader->compression = 0;
	infoHeader->hPixelsPerMeter = img.getHorizontalResolution();
	infoHeader->vPixelsPerMeter  = img.getVerticalResolution();
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
	
	/*IplImage is already padded*/
	memcpy(pixels,matrix->data.ptr,src->imageSize); 
	/*---------pixels-----------*/

	cvFlip(src,src,0);
}



Dib::Dib(unsigned rows, unsigned cols, unsigned colorBits) :
	fileHeader(NULL), colorPalette(NULL) {
	bytesPerPixel = colorBits >> 3;

	unsigned paletteSize = getPaletteSize(colorBits);

	infoHeader = new BitmapInfoHeader;
	infoHeader->size = 40;
	infoHeader->width = cols;
	infoHeader->height = rows;
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
		UA_DOUT(4, 5, "constructor: color palette created: size "
				<< paletteSize * sizeof(RgbQuad));
	}

	rowBytes = getRowBytes(infoHeader->width, infoHeader->bitCount);
	rowPaddingBytes = rowBytes - (infoHeader->width * bytesPerPixel);
	infoHeader->imageSize = infoHeader->height * rowBytes;

	isAllocated = true;
	pixels = new unsigned char[infoHeader->imageSize];
	memset(pixels, 255, infoHeader->imageSize);
	UA_DOUT(4, 5, "constructor: image size is " << infoHeader->imageSize);
}

Dib::Dib(char * filename) :
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
	delete infoHeader;

	if (colorPalette != NULL) {
		delete[] colorPalette;
	}

	if ((isAllocated) && (pixels != NULL)) {
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
void Dib::readFromHandle(HANDLE handle) {
	BITMAPINFOHEADER * dibHeaderPtr = (BITMAPINFOHEADER *) GlobalLock(handle);

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
		colorPalette = reinterpret_cast<RgbQuad *>(
				reinterpret_cast<unsigned char *>(dibHeaderPtr)
				+ sizeof(BITMAPINFOHEADER));
	}

	pixels = reinterpret_cast<unsigned char *>(dibHeaderPtr) + sizeof(BITMAPINFOHEADER)
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
void Dib::readFromFile(const char * filename) {
	UA_ASSERT_NOT_NULL(filename);

	deallocate();

	FILE * fh = fopen(filename, "rb"); // C4996
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

bool Dib::writeToFile(const char * filename) {
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

	FILE * fh = fopen(filename, "wb"); // C4996
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

unsigned char * Dib::getPixelBuffer() {
	UA_ASSERT_NOT_NULL(pixels);
	return pixels;
}


unsigned char * Dib::getRowPtr(unsigned row) {
	UA_ASSERT(row < infoHeader->height);
	return pixels + row * rowBytes;
}

void Dib::getPixel(unsigned row, unsigned col, RgbQuad & quad) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned char * ptr = pixels + row * rowBytes + col * bytesPerPixel;

	assert(infoHeader->bitCount != 8);
	quad.rgbRed = ptr[0];
	quad.rgbGreen = ptr[1];
	quad.rgbBlue = ptr[2];
	quad.rgbReserved = 0;
}

unsigned char Dib::getPixelAvgGrayscale(unsigned row, unsigned col) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned char * ptr = pixels + row * rowBytes + col * bytesPerPixel;

	if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
		return (unsigned char) (0.33333 * ptr[0] + 0.33333 * ptr[1] + 0.33333
				* ptr[2]);
	} else if (infoHeader->bitCount == 8) {
		return *ptr;
	}
	UA_ASSERTS(false, "bitCount " << infoHeader->bitCount << " not implemented yet");
	return 0;
}

unsigned char Dib::getPixelGrayscale(unsigned row, unsigned col) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned char * ptr = pixels + row * rowBytes + col * bytesPerPixel;

	if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
		return static_cast<unsigned char> (0.3333 * ptr[0] + 0.3333 * ptr[1]
				+ 0.3333 * ptr[2]);
	} else if (infoHeader->bitCount == 8) {
		return *ptr;
	}
	UA_ASSERTS(false, "bitCount " << infoHeader->bitCount << " not implemented yet");
	return 0;
}

void Dib::setPixel(unsigned x, unsigned y, RgbQuad & quad) {
	UA_ASSERT(x < infoHeader->width);
	UA_ASSERT(y < infoHeader->height);

	unsigned char * ptr = (pixels + y * rowBytes + x * bytesPerPixel);

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

void Dib::setPixelGrayscale(unsigned row, unsigned col, unsigned char value) {
	UA_ASSERT(row < infoHeader->height);
	UA_ASSERT(col < infoHeader->width);

	unsigned char * ptr = pixels + row * rowBytes + col * bytesPerPixel;

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
 * DIBs are flipped in Y;
 */
bool Dib::crop(Dib &src, unsigned x0, unsigned y0, unsigned x1, unsigned y1) {
	UA_ASSERT_NOT_NULL(src.infoHeader);
	UA_ASSERT(x1 > x0);
	UA_ASSERT(y1 > y0);

	if(infoHeader != NULL)
		delete infoHeader;
	infoHeader = new BitmapInfoHeader;
	
	*infoHeader = *src.infoHeader;
	bytesPerPixel = src.bytesPerPixel;

	if ((y0 >= infoHeader->height) || (y1 >= infoHeader->height) || (x0
			>= infoHeader->width) || (x1 >= infoHeader->width)) {
		return false;
	}

	infoHeader->width = x1 - x0;
	infoHeader->height = y1 - y0;

	rowBytes = getRowBytes(infoHeader->width, infoHeader->bitCount);
	rowPaddingBytes = rowBytes - (infoHeader->width * bytesPerPixel);

	infoHeader->imageSize = infoHeader->height * rowBytes;
	
	unsigned paletteSize = getPaletteSize(src.infoHeader->bitCount);
	if (paletteSize > 0) {
		if (this->colorPalette != NULL)
			delete [] this->colorPalette;
		this->colorPalette = new RgbQuad[paletteSize];

		memcpy(this->colorPalette, src.colorPalette, paletteSize * sizeof(RgbQuad));
	}
	if(pixels != NULL)
		delete [] pixels;
	pixels = new unsigned char[infoHeader->imageSize];
	isAllocated = true;

	unsigned char * srcRowPtr = src.pixels + (src.infoHeader->height - y1)
			* src.rowBytes + x0 * bytesPerPixel;
	unsigned char * destRowPtr = pixels;

	for (unsigned row = 0; row < infoHeader->height; ++row, srcRowPtr
			+= src.rowBytes, destRowPtr += rowBytes) {
		memcpy(destRowPtr, srcRowPtr, infoHeader->width * bytesPerPixel);
	}
	return true;
}

/*
TODO: copy over file header
*/
Dib * Dib::convertGrayscale(Dib & src) {
	UA_ASSERT(src.getBitsPerPixel() == 24 || src.getBitsPerPixel() == 8);

	if (src.getBitsPerPixel() == 8) {

		UA_DOUT(4, 9, "convertGrayscale: Already grayscale image.");
		return new Dib(src);
	}

	UA_DOUT(4, 9, "convertGrayscale: Converting from 24 bit to 8 bit.");

	// 24bpp -> 8bpp
	Dib * dibBuffer = new Dib(src.getHeight(), src.getWidth(), 8);

	dibBuffer->infoHeader->hPixelsPerMeter = src.infoHeader->hPixelsPerMeter;
	dibBuffer->infoHeader->vPixelsPerMeter = src.infoHeader->vPixelsPerMeter;

	UA_DOUT(4, 9, "convertGrayscale: Made dib");

	for (unsigned row = 0; row < src.getHeight(); ++row) {
		for (unsigned col = 0; col < src.getWidth(); ++col) {
			dibBuffer->setPixelGrayscale(row, col, src.getPixelGrayscale(row,
					col));
		}
	}

	UA_DOUT(4, 9, "convertGrayscale: Generated 8 bit grayscale image.");

	return dibBuffer;
}

void Dib::sobelEdgeDetectionWithMask(Dib & src, int mask1[3][3],
		int mask2[3][3]) {
	UA_ASSERT_NOT_NULL(src.infoHeader);

	int I, J;
	unsigned Y, X, SUM;
	long sum1, sum2;
	unsigned width = src.infoHeader->width;
	unsigned height = src.infoHeader->height;

	unsigned char * srcRowPtr = src.pixels, *destRowPtr = pixels;
	unsigned char * pixel, *pixelStart, pixelValue;

	assert(mask1 != NULL);

	for (Y = 0; Y <= height - 1; ++Y, srcRowPtr += rowBytes, destRowPtr
			+= rowBytes) {
		pixelStart = srcRowPtr;
		for (X = 0; X <= width - 1; ++X, pixelStart += bytesPerPixel) {
			sum1 = 0;
			sum2 = 0;

			/* image boundaries */
			if ((Y == 0) || (Y == height - 1) || (X == 0) || (X == width - 1)) {
				SUM = 0;
			} else {
				/* Convolution starts here */
				for (J = -1; J <= 1; ++J, pixel += rowBytes) {
					pixel = pixelStart;
					for (I = -1; I <= 1; ++I, pixel += bytesPerPixel) {
						if ((infoHeader->bitCount == 24)
								|| (infoHeader->bitCount == 32)) {
							// convert to grayscale
							pixelValue = (unsigned char) (0.3 * pixel[0] + 0.59
									* pixel[1] + 0.11 * pixel[2]);
						} else {
							pixelValue = pixel[0];
						}

						sum1 += pixelValue * mask1[I + 1][J + 1];
						if (mask2 != NULL) {
							sum2 += pixelValue * mask2[I + 1][J + 1];

						}
					}
				}

				/*---GRADIENT MAGNITUDE APPROXIMATION (Myler p.218)----*/
				SUM = abs(sum1) + abs(sum2);
			}

			if (SUM > 255)
				SUM = 255;
			if (SUM < 0)
				SUM = 0;

			pixel = destRowPtr + X * bytesPerPixel;
			pixel[0] = (unsigned char) (255 - SUM);
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
	GX[0][0] = -1;
	GX[0][1] = 0;
	GX[0][2] = 1;
	GX[1][0] = -2;
	GX[1][1] = 0;
	GX[1][2] = 2;
	GX[2][0] = -1;
	GX[2][1] = 0;
	GX[2][2] = 1;

	/* 3x3 GY Sobel mask.  Ref: www.cee.hw.ac.uk/hipr/html/sobel.html */
	GY[0][0] = 1;
	GY[0][1] = 2;
	GY[0][2] = 1;
	GY[1][0] = 0;
	GY[1][1] = 0;
	GY[1][2] = 0;
	GY[2][0] = -1;
	GY[2][1] = -2;
	GY[2][2] = -1;

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
	MASK[0][0] = -1;
	MASK[0][1] = -1;
	MASK[0][2] = -1;
	MASK[0][3] = -1;
	MASK[0][4] = -1;
	MASK[1][0] = -1;
	MASK[1][1] = -1;
	MASK[1][2] = -1;
	MASK[1][3] = -1;
	MASK[1][4] = -1;
	MASK[2][0] = -1;
	MASK[2][1] = -1;
	MASK[2][2] = 24;
	MASK[2][3] = -1;
	MASK[2][4] = -1;
	MASK[3][0] = -1;
	MASK[3][1] = -1;
	MASK[3][2] = -1;
	MASK[3][3] = -1;
	MASK[3][4] = -1;
	MASK[4][0] = -1;
	MASK[4][1] = -1;
	MASK[4][2] = -1;
	MASK[4][3] = -1;
	MASK[4][4] = -1;

	for (Y = 0; Y <= height - 1; ++Y) {
		for (X = 0; X <= width - 1; ++X) {
			SUM = 0;

			/* image boundaries */
			if ((Y <= 1) || (Y >= height - 2) || (X <= 1) || (X >= width - 2)) {
				SUM = 0;
			} else {
				/* Convolution starts here */
				for (I = -2; I <= 2; ++I) {
					for (J = -2; J <= 2; ++J) {
						SUM = SUM + src.getPixelGrayscale(Y + J, X + I)
								* MASK[I + 2][J + 2];
					}
				}
			}

			if (SUM > 255)
				SUM = 255;
			if (SUM < 0)
				SUM = 0;

			setPixelGrayscale(Y, X, (unsigned char) (255 - SUM));
		}
	}
}

/*
 *
 */
void Dib::histEqualization(Dib & src) {
	UA_ASSERT_NOT_NULL(src.infoHeader);

	unsigned char * srcRowPtr, *destRowPtr, *srcPixel, *destPixel, pixelValue;
	unsigned histogram[256];
	double totPixels = static_cast<double> (src.infoHeader->height
			* src.infoHeader->width);
	double sum[256], runningSum;
	unsigned row, col, i;
	bool isRgb = (src.infoHeader->bitCount == 24) || (src.infoHeader->bitCount
			== 32);

	copyInternals(src);

	memset(&histogram, 0, sizeof(histogram));
	memset(&sum, 0, sizeof(sum));

	srcRowPtr = src.pixels;
	destRowPtr = pixels;
	for (row = 0; row < infoHeader->height; ++row, srcRowPtr += src.rowBytes, destRowPtr
			+= rowBytes) {
		srcPixel = srcRowPtr;
		destPixel = destRowPtr;
		for (col = 0; col < infoHeader->width; ++col, srcPixel
				+= src.bytesPerPixel, destPixel += bytesPerPixel) {
			if (isRgb) {
				// convert to grayscale
				pixelValue = static_cast<unsigned char> (0.3 * srcPixel[0]
						+ 0.59 * srcPixel[1] + 0.11 * srcPixel[2]);
			} else {
				pixelValue = srcPixel[0];
			}
			histogram[pixelValue]++;

			// save grayscale value to be used in loop below
			destPixel[0] = pixelValue;
		}
	}

	for (i = 0, runningSum = 0; i <= 255; ++i) {
		runningSum += (histogram[i] / totPixels);
		sum[i] = runningSum;
	}

	destRowPtr = pixels;
	for (row = 0; row < infoHeader->height; ++row, destRowPtr += rowBytes) {
		destPixel = destRowPtr;
		for (col = 0; col < infoHeader->width; ++col, destPixel
				+= bytesPerPixel) {
			destPixel[0] = static_cast<unsigned char> (256 * sum[destPixel[0]]);
			if (isRgb) {
				destPixel[1] = destPixel[2] = destPixel[0];
			}
		}
	}
}

//http://opencv.willowgarage.com/documentation/c/basic_structures.html
//cvmat: Matrices are stored row by row. All of the rows are aligned by 4 bytes.
IplImageContainer*  Dib::generateIplImage(){

	IplImageContainer * iplContainer;
	IplImage* image = NULL;
	CvMat hdr, *matrix = NULL;
	unsigned width,height,imgsize;
	CvSize size;

	UA_ASSERTS(this->getBitsPerPixel() == 8, "createImageFromDib8U requires an unsigned 8bit image");

	width = this->getWidth();
	height = this->getHeight();
	imgsize = width*height;

	size.width = width;
	size.height = height;

	image = cvCreateImage( size, IPL_DEPTH_8U, 1 );
	matrix = cvGetMat( image, &hdr );
	
	memcpy(matrix->data.ptr,this->getPixelBuffer(),imgsize);

	cvFlip(image,image,0);

	iplContainer = new IplImageContainer(NULL);
	iplContainer->setIplImage(image);
	iplContainer->setHorizontalResolution(this->infoHeader->hPixelsPerMeter);
	iplContainer->setVerticalResolution(this->infoHeader->vPixelsPerMeter);
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
void Dib::rectangle(unsigned x, unsigned y, unsigned width, unsigned height,
		RgbQuad & quad) {
	
	this->line(x,y,x,y+height,quad);
	this->line(x,y,x+width,y,quad);
	this->line(x+width,y,x+width,y+height,quad);
	this->line(x,y+height,x+width,y+height,quad);
	
}



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
		convolve2DSlow(Dib::DPI_400_KERNEL, 3, 3);
		break;

	case 600:
		UA_DOUT(4, 5, "tpPresetFilter: Applying BLANK_KERNEL");
		convolve2DSlow(Dib::BLANK_KERNEL, 3, 3);
		UA_DOUT(4, 5, "tpPresetFilter: Applying BLUR_KERNEL");
		convolve2DSlow(Dib::BLUR_KERNEL, 3, 3);
		break;

	case 300:
		UA_DOUT(4, 5, "tpPresetFilter: No filter applied (300 dpi)");
		break;

	default:
		UA_DOUT(4, 5, "tpPresetFilter: No filter applied (default)");
		break;
	}
}


// actually slow version
void Dib::convolve2DSlow(const float(&kernel)[9], int kernelSizeX, int kernelSizeY)
{
    int i, j, m, n, mm, nn;
    int kCenterX, kCenterY;                         // center index of kernel
    float sum;                                      // temp accumulation buffer
    int rowIndex, colIndex;

	UA_ASSERT(getBitsPerPixel() == 8);
	UA_ASSERT(infoHeader->imageSize > 0);

	int dataSizeX = (int) infoHeader->width ;
	int dataSizeY = (int) infoHeader->height ;

	unsigned char * buffer = new unsigned char[infoHeader->imageSize];

    // find center position of kernel (half of kernel size)
    kCenterX = kernelSizeX / 2;
    kCenterY = kernelSizeY / 2;

    for(i=0; i < dataSizeY; ++i)                // rows
    {
        for(j=0; j < dataSizeX; ++j)            // columns
        {
            sum = 0;                            // init to 0 before sum
            for(m=0; m < kernelSizeY; ++m)      // kernel rows
            {
                mm = kernelSizeY - 1 - m;       // row index of flipped kernel

                for(n=0; n < kernelSizeX; ++n)  // kernel columns
                {
                    nn = kernelSizeX - 1 - n;   // column index of flipped kernel

                    // index of input signal, used for checking boundary
                    rowIndex = i + m - kCenterY;
                    colIndex = j + n - kCenterX;

                    // ignore input samples which are out of bound
                    if(rowIndex >= 0 && rowIndex < dataSizeY && colIndex >= 0 && colIndex < dataSizeX)
                        sum += pixels[rowBytes * rowIndex + colIndex] * kernel[kernelSizeX * mm + nn];
                }
            }
			if(sum > 0)
				buffer[i*rowBytes + j] = (unsigned char)(sum + 0.5f);
			else
				buffer[i*rowBytes + j] = (unsigned char)(-sum + 0.5f);
        }
    }
	memcpy(pixels,buffer,infoHeader->imageSize);
	delete [] buffer;
}

void Dib::gaussianBlur(Dib & src) {
	UA_ASSERT_NOT_NULL(src.infoHeader);
	copyInternals(src);

	unsigned char * tmpPixels = new unsigned char[infoHeader->imageSize];
	UA_ASSERT_NOT_NULL(tmpPixels);

	unsigned char * srcRowPtr, *destRowPtr;
	unsigned i, j, k, x, y, sumr, sumg, sumb;
	unsigned char * pixel;

	for (i = 0; i < infoHeader->width - 1; ++i) {
		srcRowPtr = src.pixels;
		destRowPtr = tmpPixels;
		for (j = 0; j < infoHeader->height - 1; ++j, srcRowPtr += src.rowBytes, destRowPtr
				+= rowBytes) {
			sumr = 0;
			sumg = 0;
			sumb = 0;
			for (k = 0; k < GAUSS_WIDTH; ++k) {
				x = i - ((GAUSS_WIDTH - 1) >> 1) + k;

				if ((x >= 0) && (x < infoHeader->width)) {
					pixel = &srcRowPtr[x * bytesPerPixel];
					sumr += pixel[0] * GAUSS_FACTORS[k];
					if ((infoHeader->bitCount == 24) || (infoHeader->bitCount
							== 32)) {
						sumg += pixel[1] * GAUSS_FACTORS[k];
						sumb += pixel[2] * GAUSS_FACTORS[k];
					}
				}
			}

			pixel = &destRowPtr[i * bytesPerPixel];
			pixel[0] = sumr / GAUSS_SUM;
			if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
				pixel[1] = sumg / GAUSS_SUM;
				pixel[2] = sumb / GAUSS_SUM;
			}
		}
	}

	for (i = 0; i < infoHeader->width - 1; ++i) {
		destRowPtr = pixels;
		for (j = 0; j < infoHeader->height - 1; ++j, destRowPtr += rowBytes) {
			sumr = 0;
			sumg = 0;
			sumb = 0;

			for (k = 0; k < GAUSS_WIDTH; ++k) {
				y = j - ((GAUSS_WIDTH - 1) >> 1) + k;

				if ((y >= 0) && (y < infoHeader->height)) {
					pixel = &tmpPixels[y * rowBytes + i * bytesPerPixel];
					sumr += pixel[0] * GAUSS_FACTORS[k];
					if ((infoHeader->bitCount == 24) || (infoHeader->bitCount
							== 32)) {
						sumg += pixel[1] * GAUSS_FACTORS[k];
						sumb += pixel[2] * GAUSS_FACTORS[k];
					}
				}
			}

			pixel = &destRowPtr[i * bytesPerPixel];
			pixel[0] = sumr / GAUSS_SUM;
			if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
				pixel[1] = sumg / GAUSS_SUM;
				pixel[2] = sumb / GAUSS_SUM;
			}
		}
	}
}

/*
 * Based on Adam Milstein's code
 */
void Dib::blur(Dib & src) {
	UA_ASSERT_NOT_NULL(src.infoHeader);
	double radius = 5.0; //UNSHARP_RAD;
	int *m_FilterVector;
	int m_Denominator;
	unsigned m_Dim;
	unsigned d = (int) (5.0f * radius + 1.0f);
	int i2, v;
	int k;

	copyInternals(src);

	// SetRadius
	d |= 1; // must be odd
	m_Dim = d;

	m_FilterVector = new int[m_Dim];
	d /= 2;

	double num = 2 * radius * radius;
	double f = exp(d * d / num);
	m_Denominator = (int) f;

	m_FilterVector[d] = m_Denominator;

	for (unsigned i = 1; i <= d; i++) {
		i2 = -(int) (i * i);
		v = (int) (f * exp(i2 / num));
		m_FilterVector[d - i] = v;
		m_FilterVector[d + i] = v;
		m_Denominator += 2 * v;
	}

	d = m_Dim / 2;
	bool bHorizontal;
	unsigned line;
	unsigned pxl;

	unsigned char * pStartSrc = (unsigned char *) src.pixels;
	unsigned char * pStartDest = (unsigned char *) pixels;
	unsigned char * pLineSrc;
	unsigned char * pLineDest;
	unsigned char * pPixelDest;
	int * pFactors;
	unsigned xBegin;
	unsigned xEnd;
	int denom;
	int sum;
	unsigned char * pPixelSrc;
	unsigned x;

	unsigned nLines; // number of lines (horizontal or vertical)
	unsigned nPixels; // number of pixels per line
	unsigned dPixelSrc; // pixel step in source
	unsigned dPixelDest; // pixel step in destination
	unsigned dLineSrc; // line step in source
	unsigned dLineDest; // line step in destination
	unsigned char srcPixelValue;

	for (k = 0; k < 2; k++) {
		if (k == 0)
			bHorizontal = true;
		else
			bHorizontal = false;

		if (bHorizontal) {
			nLines = infoHeader->height - 1;
			nPixels = infoHeader->width;
			dPixelSrc = 3;
			dPixelDest = 3;
			dLineSrc = src.rowBytes;
			dLineDest = rowBytes;
		} else {
			nLines = infoHeader->width;
			nPixels = infoHeader->height - 1;
			dPixelSrc = src.rowBytes;
			dPixelDest = rowBytes;
			dLineSrc = 3;
			dLineDest = 3;
		}

		if (d > nPixels / 2) {
			d = nPixels / 2;
		}

		pLineSrc = pStartSrc;
		pLineDest = pStartDest;

		for (line = 0; line < nLines; line++, pLineSrc += dLineSrc, pLineDest
				+= dLineDest) {
			// loop through lines
			pPixelDest = pLineDest;

			for (pxl = 0, pPixelSrc = pLineSrc; pxl < d; pxl++, pPixelDest
					+= dPixelDest) {
				// loop through pixels in left/top margin
				pFactors = m_FilterVector + d - pxl;

				xEnd = pxl + d;
				if (xEnd > nPixels)
					xEnd = nPixels;

				denom = 0;
				sum = 0;

				for (x = 0; x < xEnd; x++, pPixelSrc += dPixelSrc) {
					denom += *pFactors;
					if ((infoHeader->bitCount == 24) || (infoHeader->bitCount
							== 32)) {
						srcPixelValue = static_cast<unsigned char> (0.3
								* pPixelSrc[0] + 0.59 * pPixelSrc[1] + 0.11
								* pPixelSrc[2]);
					} else {
						srcPixelValue = pPixelSrc[0];
					}
					sum += *pFactors++ * srcPixelValue;
				}

				if (denom)
					sum /= denom;
				pPixelDest[0] = static_cast<unsigned char> (sum);
				if ((infoHeader->bitCount == 24)
						|| (infoHeader->bitCount == 32)) {
					pPixelDest[1] = pPixelDest[2] = pPixelDest[0];
				}
			}

			for (pxl = d; pxl < nPixels - d; pxl++, pPixelDest += dPixelDest) {
				// loop through pixels in main area
				pFactors = m_FilterVector;
				sum = 0;

				for (x = pxl - d, pPixelSrc = &pLineSrc[x * dPixelSrc]; x
						<= pxl + d; x++, pPixelSrc += dPixelSrc) {
					if ((infoHeader->bitCount == 24) || (infoHeader->bitCount
							== 32)) {
						srcPixelValue = static_cast<unsigned char> (0.3
								* pPixelSrc[0] + 0.59 * pPixelSrc[1] + 0.11
								* pPixelSrc[2]);
					} else {
						srcPixelValue = pPixelSrc[0];
					}
					sum += *pFactors++ * srcPixelValue;
				}

				sum /= m_Denominator;
				pPixelDest[0] = static_cast<unsigned char> (sum);
				if ((infoHeader->bitCount == 24)
						|| (infoHeader->bitCount == 32)) {
					pPixelDest[1] = pPixelDest[2] = pPixelDest[0];
				}
			}

			for (pxl = nPixels - d; pxl < nPixels; pxl++, pPixelDest
					+= dPixelDest) {
				// loop through pixels in right/bottom margin
				pFactors = m_FilterVector;
				denom = 0;
				sum = 0;

				xBegin = pxl - d;
				if (xBegin < 0)
					xBegin = 0;

				for (x = xBegin, pPixelSrc = &pLineSrc[x * dPixelSrc]; x
						< nPixels; x++, pPixelSrc += dPixelSrc) {
					denom += *pFactors;
					if ((infoHeader->bitCount == 24) || (infoHeader->bitCount
							== 32)) {
						srcPixelValue = static_cast<unsigned char> (0.3
								* pPixelSrc[0] + 0.59 * pPixelSrc[1] + 0.11
								* pPixelSrc[2]);
					} else {
						srcPixelValue = pPixelSrc[0];
					}
					srcPixelValue = pPixelSrc[0];
					sum += *pFactors++ * srcPixelValue;
				}

				if (denom)
					sum /= denom;
				pPixelDest[0] = static_cast<unsigned char> (sum);
				if ((infoHeader->bitCount == 24)
						|| (infoHeader->bitCount == 32)) {
					pPixelDest[1] = pPixelDest[2] = pPixelDest[0];
				}
			}
		} // next line

		pStartSrc++;
		pStartDest++;
	}

	delete[] m_FilterVector;
}

void Dib::unsharp(Dib & src) {
	UA_ASSERT_NOT_NULL(src.infoHeader);
	double depth = UNSHARP_DEPTH;
	int denom;

	denom = 10000; // use an arbitrary denominator, not too small
	int dpt = (int) ((double) denom * depth);
	int v, dptplus = dpt + denom;

	copyInternals(src);

	unsigned width = src.infoHeader->width;
	unsigned height = src.infoHeader->height;
	unsigned Y, X;
	unsigned char * srcRowPtr = src.pixels, *destRowPtr = pixels;
	unsigned char * srcPixel, *destPixel, srcPixelValue, destPixelValue;

	for (Y = 0; Y <= height - 1; ++Y, srcRowPtr += rowBytes, destRowPtr
			+= rowBytes) {
		srcPixel = srcRowPtr;
		destPixel = destRowPtr;

		for (X = 0; X <= width - 1; ++X, srcPixel += bytesPerPixel, destPixel
				+= bytesPerPixel) {
			if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
				srcPixelValue = (unsigned char) (0.3 * srcPixel[0] + 0.59
						* srcPixel[1] + 0.11 * srcPixel[2]);
				destPixelValue = (unsigned char) (0.3 * destPixel[0] + 0.59
						* destPixel[1] + 0.11 * destPixel[2]);
			} else {
				srcPixelValue = srcPixel[0];
				destPixelValue = destPixel[0];
			}

			v = dptplus * srcPixelValue - dpt * destPixelValue;
			v /= denom;

			// Clipping is very essential here. for large values of depth
			// (> 5.0f) more than half of the pixel values are clipped.
			if (v > 255)
				v = 255;
			if (v < 0)
				v = 0;
			destPixel[0] = (unsigned char) v;
			if ((infoHeader->bitCount == 24) || (infoHeader->bitCount == 32)) {
				destPixel[1] = destPixel[2] = destPixel[0];
			}
		}
	}
}

unsigned Dib::getDpi() {
	// 1 inch = 0.0254 meters
	return static_cast<unsigned> (infoHeader->hPixelsPerMeter * 0.0254 + 0.5);
}


struct coordinate
{
	unsigned int x, y;
	void * data;
};

struct lineBlob
{
	unsigned int min, max;
	unsigned int blobId;

	bool attached;
};

struct blob
{
	//unsigned int blobId;
	coordinate min, max;

	coordinate center;
};


void detectBlobs(Dib & frame, Dib & finalFrame)
{
	int blobCounter = 0;
	map<unsigned int, blob> blobs;

    unsigned char threshold = 120;


	UA_DOUT(4, 5, "DETECT BLOBS");

	UA_DOUT(4, 5, "frame : " << "width/"  << (int)frame.getWidth() << " height/"  << (int)frame.getHeight());
	vector< vector<lineBlob> > imgData(frame.getWidth());

	for(int row = 0; row < (int)frame.getHeight(); ++row)
	{

		for(int column = 0; column < (int)frame.getWidth(); ++column)
		{

			//unsigned char byte = (unsigned char) imgStream.get();
			unsigned char byte = (unsigned char) frame.getPixelBuffer()[(row*frame.getWidth())+ column];

			if(byte >= threshold)
			{

				int start = column;

				for(;byte >= threshold; byte = (unsigned char) frame.getPixelBuffer()[(row*frame.getWidth())+ column], ++column);

				int stop = column-1;
				lineBlob lineBlobData = {start, stop, blobCounter, false};

				imgData[row].push_back(lineBlobData);
				blobCounter++;
			}
		}
	}
	UA_DOUT(4, 5, "imgdata : " << "size/"  << (int)imgData[0].size());

	/* Check lineBlobs for a touching lineblob on the next row */

	for(int row = 0; row <  (int) imgData.size(); ++row)
	{

		for(int entryLine1 = 0; entryLine1 < (int) imgData[row].size(); ++entryLine1)
		{

			for(int entryLine2 = 0; entryLine2 <(int)  imgData[row+1].size(); ++entryLine2)
			{

				if(!((imgData[row][entryLine1].max < imgData[row+1][entryLine2].min) || (imgData[row][entryLine1].min > imgData[row+1][entryLine2].max)))
				{

					if(imgData[row+1][entryLine2].attached == false)
					{

						imgData[row+1][entryLine2].blobId = imgData[row][entryLine1].blobId;

						imgData[row+1][entryLine2].attached = true;
					}
					else

					{
						imgData[row][entryLine1].blobId = imgData[row+1][entryLine2].blobId;

						imgData[row][entryLine1].attached = true;
					}
				}
			}
		}
	}

	// Sort and group blobs

	for(int row = 0; row < (int) imgData.size(); ++row)
	{

		for(int entry = 0; entry < (int) imgData[row].size(); ++entry)
		{

			if(blobs.find(imgData[row][entry].blobId) == blobs.end()) // Blob does not exist yet

			{
				blob blobData = {{imgData[row][entry].min, row}, {imgData[row][entry].max, row}, {0,0}};

				blobs[imgData[row][entry].blobId] = blobData;
			}
			else

			{
				if(imgData[row][entry].min < blobs[imgData[row][entry].blobId].min.x)

					blobs[imgData[row][entry].blobId].min.x = imgData[row][entry].min;

				else if(imgData[row][entry].max > blobs[imgData[row][entry].blobId].max.x)

					blobs[imgData[row][entry].blobId].max.x = imgData[row][entry].max;

				if((unsigned)row < blobs[imgData[row][entry].blobId].min.y)

					blobs[imgData[row][entry].blobId].min.y = row;

				else if((unsigned)row > blobs[imgData[row][entry].blobId].max.y)

					blobs[imgData[row][entry].blobId].max.y = row;
			}
		}
	}

	UA_DOUT(4, 5, "Blobs found: " << blobs.size());

	// Calculate center
	for(map<unsigned int, blob>::iterator i = blobs.begin(); i != blobs.end(); ++i)
	{
		(*i).second.center.x = (*i).second.min.x + ((*i).second.max.x - (*i).second.min.x) / 2;
		(*i).second.center.y = (*i).second.min.y + ((*i).second.max.y - (*i).second.min.y) / 2;

		int size = ((*i).second.max.x - (*i).second.min.x) * ((*i).second.max.y - (*i).second.min.y);

		// Print coordinates on image, if it is large enough
		if(size > 0)
		{

			RgbQuad highlightQuad;
			highlightQuad.set(255, 255, 255);

			finalFrame.line((*i).second.min.x, frame.getHeight()-(*i).second.min.y, (*i).second.max.x, frame.getHeight()-(*i).second.min.y , highlightQuad);
			finalFrame.line((*i).second.min.x, frame.getHeight()-(*i).second.max.y, (*i).second.max.x, frame.getHeight()-(*i).second.max.y , highlightQuad);
			finalFrame.line((*i).second.min.x, frame.getHeight()-(*i).second.min.y, (*i).second.min.x, frame.getHeight()-(*i).second.max.y, highlightQuad);
			finalFrame.line((*i).second.max.x, frame.getHeight()-(*i).second.min.y, (*i).second.max.x, frame.getHeight()-(*i).second.max.y, highlightQuad);
			//char textBuffer[128];
			//sprintf(textBuffer, "(%d, %d)", (*i).second.center.x, (*i).second.center.y);
		}
	}
}
