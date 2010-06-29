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
