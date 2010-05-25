#ifndef __INCLUDE_DIB_H
#define __INCLUDE_DIB_H

#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#endif

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
		rgbRed = rgbGreen = rgbBlue = 0;
	}
	RgbQuad(unsigned char r, unsigned char g, unsigned char b) {
		rgbRed = r; rgbGreen = g; rgbBlue = b;
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
	Dib(unsigned rows, unsigned cols, unsigned colorBits);
	Dib(char * filename);
	~Dib();
	void readFromFile(const char * filename) ;
	bool writeToFile(const char * filename);


	static Dib * convertGrayscale(Dib & src);
	static void tpPresetFilter(Dib * img);
	static void convolve2DFast( Dib * img, const float(&kernel) [9], int kernelSizeX, int kernelSizeY);

#ifdef WIN32
	void readFromHandle(HANDLE handle);
#endif

	unsigned getHeight();
	unsigned getWidth();
	unsigned getRowPadBytes();
	unsigned getBitsPerPixel();
	unsigned char * getPixelBuffer();
	unsigned char * getRowPtr(unsigned row);
	void getPixel(unsigned row, unsigned col, RgbQuad & quad);
	unsigned char getPixelAvgGrayscale(unsigned row, unsigned col);
	unsigned char getPixelGrayscale(unsigned row, unsigned col);
	void setPixel(unsigned row, unsigned col, RgbQuad & quad);
	void setPixelGrayscale(unsigned row, unsigned col, unsigned char value);
	unsigned char * getPixelsNoPadding();
	void setPixelsNoPadding(unsigned char * pixels);
	bool crop(Dib &src, unsigned x0, unsigned y0, unsigned x1, unsigned y1);

	void sobelEdgeDetectionWithMask(Dib & src, int mask1[3][3],
			int mask2[3][3]);

	void sobelEdgeDetection(Dib & src);
	void laplaceEdgeDetection(Dib & src);
	void histEqualization(Dib & src);
	void line(unsigned x0, unsigned y0, unsigned x1, unsigned y1, RgbQuad & quad);
	void grayscale(Dib & src);
	void gaussianBlur(Dib & src);
	void blur(Dib & src);
	void unsharp(Dib & src);
	void expandColours(int start, int end);
	unsigned getDpi();

private:
	static const double UNSHARP_RAD;
    static const double UNSHARP_DEPTH;
    static const unsigned GAUSS_WIDTH;
    static const unsigned GAUSS_FACTORS[];
    static const unsigned GAUSS_SUM;

	static const float BLUR_KERNEL[9];
	static const float BLANK_KERNEL[9];
	static const float DPI_400_KERNEL[9];

	BitmapFileHeader * fileHeader;
	BitmapInfoHeader * infoHeader;
	RgbQuad * colorPalette;
	unsigned bytesPerPixel;
	unsigned rowBytes;
	unsigned rowPaddingBytes;
	unsigned char * pixels;
	bool isAllocated;

	unsigned idx, dupe; // used by line drawing

	void copyInternals(Dib & src);
	unsigned getPaletteSize(unsigned bitCount);
	void setPalette();
	void setPalette(RgbQuad * palette);
	unsigned getRowBytes(unsigned width, unsigned bitCount);
};



#endif /* __INCLUDE_DIB_H */
