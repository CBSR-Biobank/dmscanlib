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
	unsigned char rgbRed;
	unsigned char rgbGreen;
	unsigned char rgbBlue;
	unsigned char rgbReserved;
};

class Dib {
public:
	Dib();
	Dib(unsigned rows, unsigned cols, unsigned colorBits);
	Dib(char * filename);
	~Dib();
	void readFromFile(const char * filename) ;
	void writeToFile(const char * filename);

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
	unsigned char getPixelGrayscale(unsigned row, unsigned col);
	void setPixel(unsigned row, unsigned col, RgbQuad & quad);
	void setPixelGrayscale(unsigned row, unsigned col, unsigned char value);
	unsigned char * getPixelsNoPadding();
	void setPixelsNoPadding(unsigned char * pixels);
	void crop(Dib &src, unsigned r1, unsigned c1, unsigned r2, unsigned c2);
	void convertGrayscale(Dib & src);
	void sobelEdgeDetectionWithMask(Dib & src, int mask1[3][3],
			int mask2[3][3]);
	void sobelEdgeDetection(Dib & src);
	void laplaceEdgeDetection(Dib & src);
	void histEqualization(Dib & src);
	void line(unsigned x0, unsigned y0, unsigned x1, unsigned y1, RgbQuad & quad);

private:
	BitmapFileHeader * fileHeader;
	BitmapInfoHeader * infoHeader;
	unsigned bytesPerPixel;
	unsigned rowPaddingBytes;
	unsigned char * pixels;
	bool isAllocated;

	void copyInternals(Dib & src);

	unsigned idx, dupe; // used by line drawing

};

#endif /* __INCLUDE_DIB_H */
