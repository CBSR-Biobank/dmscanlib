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

namespace dmscanlib {

class Decoder;
class Dib;
struct RgbQuad;
class PalletGrid;

class WellDecoder : public ::OpenThreads::Thread {
public:
	WellDecoder(const Decoder & decoder, const WellRectangle<unsigned> & _wellRectangle);

	virtual ~WellDecoder();

	virtual void run();

	bool isFinished();

	void decodeCallback(std::string & decodedMsg, Point<unsigned>(&corners)[4]);

	const std::string & getLabel() const {
		return wellRectangle.getLabel();
	}

	const std::string & getMessage() const {
		return message;
	}

	void setMessage(const char * message, int messageLength);

	const Rect<unsigned> & getWellRectangle() const {
		return wellRectangle.getRectangle();
	}

	const Rect<unsigned> & getDecodedRectangle() const;

	void setDecodeRectangle(const Rect<double> & rect, int scale);

	const bool getDecodeValid() {
		return message.empty();
	}

private:
	const Decoder & decoder;
	const WellRectangle<unsigned> & wellRectangle;
	std::unique_ptr<const Dib> wellImage;
	std::unique_ptr<const BoundingBox<unsigned> > boundingBox;
	std::unique_ptr<const Rect<unsigned> > decodedRect;
	std::string message;

	friend std::ostream & operator<<(std::ostream & os, const WellDecoder & m);
};

} /* namespace */

#endif /* __INC_PALLET_CELL_H */
