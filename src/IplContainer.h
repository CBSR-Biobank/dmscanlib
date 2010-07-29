#ifndef __INCLUDE_IPLCONTAINER_H
#define __INCLUDE_IPLCONTAINER_H
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