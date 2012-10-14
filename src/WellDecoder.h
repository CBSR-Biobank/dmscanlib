#ifndef __INC_PALLET_CELL_H
#define __INC_PALLET_CELL_H

#include "WellRectangle.h"
#include "geometry.h"

#include <dmtx.h>
#include <string>
#include <ostream>
#include <memory>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Thread>

using namespace std;

class Decoder;
class Dib;
struct RgbQuad;
class PalletGrid;

class WellDecoder : public OpenThreads::Thread {
public:
	WellDecoder(const Decoder & decoder,
			const WellRectangle<unsigned> & _wellRectangle);

   virtual ~WellDecoder();

   virtual void run();

   bool isFinished();

   void decodeCallback(std::string & decodedMsg, Point<unsigned>(&corners)[4]);

	const string & getLabel() {
		return wellRectangle.getLabel();
	}

	const string & getMessage() const {
		return message;
	}

	void setMessage(const char * message, int messageLength);

   const Rect<unsigned> & getWellRectangle() const {
      return wellRectangle.getRectangle();
   }

   const Rect<unsigned> & getDecodedRectangle() const {
	   return decodedRect;
   }

   const bool getDecodeValid() {
	   return message.empty();
   }

   const std::string & getBarcodeMsg();

	void setCorner(unsigned cornerId, unsigned x, unsigned y);

   void drawCellBox(Dib & image, const RgbQuad & color) const;

   void drawBarcodeBox(Dib & image, const RgbQuad & color) const;

   void writeImage(std::string basename);

private:
   const Decoder & decoder;
   unique_ptr<const Dib> wellImage;
   const WellRectangle<unsigned> & wellRectangle;
   Rect<unsigned> decodedRect;
   string message;

   friend std::ostream & operator<<(std::ostream & os, WellDecoder & m);
};

#endif /* __INC_PALLET_CELL_H */
