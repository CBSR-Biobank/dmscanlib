#ifndef __INCLUDE_IPLCONTAINER_H
#define __INCLUDE_IPLCONTAINER_H
#include "cv.h"

class IplImageContainer {

private:
	IplImage * image;
	unsigned horizontalResolution;
	unsigned verticalResolution;

public:
	IplImageContainer(IplImage * image);
	IplImageContainer();
	~IplImageContainer();
	IplImage * getIplImage();
	void setIplImage(IplImage * image);
	unsigned getHorizontalResolution();
	unsigned getVerticalResolution();
	void setHorizontalResolution(unsigned pixelPerMeter);
	void setVerticalResolution(unsigned pixelPerMeter);

};

#endif