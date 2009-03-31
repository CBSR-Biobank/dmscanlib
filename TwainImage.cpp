#include "TwainImage.h"

//TODO: add exceptions
TwainImage::TwainImage(int w, int h)
{
	if(w <= 0) w=1; //this should really be an exception
	width = w;

	if(h <= 0) h = 1;//exception here
	height = h;

	//allocate memory for the pixel array
	pixelArray = new int[width * height];
}
/*
TwainImage::TwainImage(const TwainImage& toCopy)
{
	width = toCopy.getWidth();
	height = toCopy.getHeight();
	
	//allocate the memory for the pixels and copy it over
	pixelArray = new int[width * height];
	memcpy(pixelArray, toCopy.getPixels(), sizeof(int) * width * height);
}
*/
TwainImage::~TwainImage()
{
	delete pixelArray;
}

void TwainImage::setPixels(int *pixels)
{
	memcpy(pixelArray, pixels, sizeof(int) * width * height);
}

int TwainImage::getWidth()
{
	return width;
}

int TwainImage::getHeight()
{
	return height;
}

int* TwainImage::getPixels()
{
	return pixelArray;
}


	