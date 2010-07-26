/*
Scanlib is a software library and standalone application that scans 
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
#include "IplContainer.h"

IplImageContainer::IplImageContainer(IplImage * img){
	this->image = img;
	this->horizontalResolution = 0;
	this->verticalResolution = 0;
}
IplImageContainer::IplImageContainer(){
	this->image = NULL;
	this->horizontalResolution = 0;
	this->verticalResolution = 0;
}

IplImageContainer::~IplImageContainer(){
	if(this->image != NULL){
		cvReleaseImage(&this->image);
	}
}
IplImage * IplImageContainer::getIplImage(){
	return this->image;
}
void IplImageContainer::setIplImage(IplImage * img){
	if(this->image != NULL){
		delete this->image;
	}
	this->image = img;
}

unsigned IplImageContainer::getHorizontalResolution(){
	return this->horizontalResolution;
}
unsigned IplImageContainer::getVerticalResolution(){
	return this->verticalResolution;
}
void IplImageContainer::setHorizontalResolution(unsigned pixMeter){
	this->horizontalResolution = pixMeter;
}
void IplImageContainer::setVerticalResolution(unsigned pixMeter){
	this->verticalResolution = pixMeter;
}
