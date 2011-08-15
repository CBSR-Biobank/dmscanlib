#ifndef __INC_PALLET_CELL_H
#define __INC_PALLET_CELL_H

#include "cxtypes.h"

#ifdef _VISUALC_
#   include <memory>
#else
#   include <tr1/memory>
#endif

class Dib;

class PalletCell {
public:
	PalletCell(std::tr1::shared_ptr<Dib> img, unsigned row, unsigned col);
	~PalletCell();

	std::tr1::shared_ptr<const Dib> getImage() { return image; };

private:
	std::tr1::shared_ptr<const Dib> image;
	unsigned row;
	unsigned col;
	CvRect barcodePos;
};


#endif /* __INC_PALLET_CELL_H */
