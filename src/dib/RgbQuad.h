#ifndef __INCLUDE_RGB_QUAD_H
#define __INCLUDE_RGB_QUAD_H

namespace dmscanlib {

/* 
 * Colour palette
 */
class RgbQuad {
public:
   RgbQuad();
   RgbQuad(const RgbQuad & other);
   RgbQuad(unsigned char r, unsigned char g, unsigned char b);
   void set(unsigned char r, unsigned char g, unsigned char b);
   unsigned toUnsignedInt();
   void scale(double factor);

   unsigned char getRed() const { return rgbRed; }
   unsigned char getGreen() const { return rgbGreen; }
   unsigned char getBlue() const { return rgbBlue; }

private:
   unsigned char rgbRed;
   unsigned char rgbGreen;
   unsigned char rgbBlue;
   unsigned char rgbReserved;
};

} /* namespace */


#endif /*  __INCLUDE_RGB_QUAD_H */
