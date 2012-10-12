#ifndef __INC_PALLET_CELL_H
#define __INC_PALLET_CELL_H

#include "DecodedWell.h"
#include "WellRectangle.h"
#include "geometry.h"

#include <dmtx.h>
#include <string>
#include <ostream>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Thread>

#ifdef _VISUALC_
#   include <memory>
#else
#   include <tr1/memory>
#endif

using namespace std;

class Decoder;
class Dib;
struct RgbQuad;
class PalletGrid;

class WellDecoder : public OpenThreads::Thread {
public:
   WellDecoder(Decoder & decoder, WellRectangle<int> & _wellCoordinates);

   virtual ~WellDecoder();

   virtual void run();

   bool isFinished();

   void decodeCallback(std::string & decodedMsg, Point<unsigned>(&corners)[4]);

   std::tr1::shared_ptr<const Dib> getImage() const;

   const WellRectangle<int> & geRectangle() const {
      return wellRectangle;
   }

   const bool getDecodeValid() {
      return !decodedWell.getMessage().empty();
   }

   const std::string & getBarcodeMsg();

   void drawCellBox(Dib & image, const RgbQuad & color) const;

   void drawBarcodeBox(Dib & image, const RgbQuad & color) const;

   void writeImage(std::string basename);

private:

   Decoder & decoder;
   WellRectangle<int> & wellRectangle;
   std::tr1::shared_ptr<const Dib> cellImage;
   DecodedWell decodedWell;

   friend std::ostream & operator<<(std::ostream & os, WellDecoder & m);
};

#endif /* __INC_PALLET_CELL_H */
