#ifndef __INC_PALLET_CELL_H
#define __INC_PALLET_CELL_H

#include "DecodeResult.h"

#include <opencv/cv.h>
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

class Dib;
struct RgbQuad;
class PalletGrid;

class PalletCell : public OpenThreads::Thread {
public:
	PalletCell(PalletGrid & grid, unsigned row, unsigned col,
			CvRect & parentPos);

	virtual ~PalletCell();

    virtual void run();

    bool isFinished();

    void decodeCallback(std::string & decodedMsg, CvPoint(&corners)[4]);

	std::tr1::shared_ptr<const Dib> getImage();

	const CvRect & getParentPos() const {
	    return parentRect;
	}

	const unsigned getRow() const {
		return row;
	}

	const unsigned getCol() const {
		return col;
	}

	const bool getDecodeValid() {
		return !decodeResult.msg.empty();
	}

	const std::string & getBarcodeMsg();

	void drawCellBox(Dib & image, const RgbQuad & color) const;

	void drawBarcodeBox(Dib & image, const RgbQuad & color) const;

    void writeImage(std::string basename);

private:

	PalletGrid & grid;
	const unsigned row;
	const unsigned col;
	CvRect parentRect;
	std::tr1::shared_ptr<const Dib> cellImage;
	DecodeResult decodeResult;

	friend std::ostream & operator<<(std::ostream & os, PalletCell & m);
};

#endif /* __INC_PALLET_CELL_H */
