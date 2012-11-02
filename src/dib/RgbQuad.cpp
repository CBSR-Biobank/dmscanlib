/*******************************************************************************
 *
 * Device Independent Bitmap
 *
 ******************************************************************************
 *
 *****************************************************************************/


#include "RgbQuad.h"


namespace dmscanlib {

RgbQuad::RgbQuad() {
   set(0, 0, 0);
}


RgbQuad::RgbQuad(const RgbQuad & other) {
   rgbRed = other.rgbRed;
   rgbGreen = other.rgbGreen;
   rgbBlue = other.rgbBlue;
}

RgbQuad::RgbQuad(unsigned char r, unsigned char g, unsigned char b) {
   set(r, g, b);
}

void RgbQuad::set(unsigned char r, unsigned char g, unsigned char b) {
   rgbRed = r;
   rgbGreen = g;
   rgbBlue = b;
   rgbReserved = 0;
}

unsigned RgbQuad::toUnsignedInt() {
   return rgbRed + rgbGreen + rgbBlue;
}

void RgbQuad::scale(double factor) {
   rgbRed   = static_cast<unsigned>(rgbRed * factor);
   rgbGreen = static_cast<unsigned>(rgbGreen * factor);
   rgbBlue  = static_cast<unsigned>(rgbBlue * factor);
}


} /* namespace */
