#ifndef TWAINIMAGE_H
#define TWAINIMAGE_H

#include <memory.h>
#include "TwainException.h"

class TwainImage
{
public:
	TwainImage(int w, int h);
	TwainImage(const TwainImage& toCopy);
	~TwainImage();
	void setPixels(int *pixels);
	int getHeight();
	int getWidth();
	int* getPixels();

private:
	int width;
	int height;
	int* pixelArray;
};

#endif