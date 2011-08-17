#ifndef __INC_PALLET_CELL_H
#define __INC_PALLET_CELL_H

#include "cv.h"
#include "dmtx.h"

#include <string>
#include <ostream>

#ifdef _VISUALC_
#   include <memory>
#else
#   include <tr1/memory>
#endif

class Dib;
class RgbQuad;

class PalletCell {
public:
	PalletCell(std::tr1::shared_ptr<Dib> img, unsigned row, unsigned col,
			CvRect & parentPos);
	~PalletCell();

	std::tr1::shared_ptr<const Dib> getImage() {
		return image;
	}
	;

	std::tr1::shared_ptr<const CvRect> getParentPos();

	const unsigned getRow() {
		return row;
	}

	const unsigned getCol() {
		return col;
	}

	const bool getDecodeValid() {
		return !decodedMsg.empty();
	}

	const std::string & getBarcodeMsg();

	void setDecodeInfo(std::string & decodedMsg, CvPoint(&corners)[4]);

	void drawCellBox(Dib & image, const RgbQuad & color) const;

	void drawBarcodeBox(Dib & image, const RgbQuad & color) const;

private:
	std::tr1::shared_ptr<const Dib> image;
	const unsigned row;
	const unsigned col;
	CvRect parentRect;
	std::string decodedMsg;
	CvPoint barcodeCorners[4];

	friend std::ostream & operator<<(std::ostream & os, PalletCell & m);
};

#endif /* __INC_PALLET_CELL_H */
