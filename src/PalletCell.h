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
class PalletGrid;

class PalletCell {
public:
	PalletCell(PalletGrid & grid, unsigned row, unsigned col,
			CvRect & parentPos);
	~PalletCell();

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
		return !decodedMsg.empty();
	}

	const std::string & getBarcodeMsg();

	void setDecodeInfo(std::string & decodedMsg, CvPoint(&corners)[4]);

	void drawCellBox(Dib & image, const RgbQuad & color) const;

	void drawBarcodeBox(Dib & image, const RgbQuad & color) const;

private:
	PalletGrid & grid;
	const unsigned row;
	const unsigned col;
	CvRect parentRect;
	std::tr1::shared_ptr<const Dib> cellImage;
	std::string decodedMsg;
	CvPoint barcodeCorners[4];

	friend std::ostream & operator<<(std::ostream & os, PalletCell & m);
};

#endif /* __INC_PALLET_CELL_H */
