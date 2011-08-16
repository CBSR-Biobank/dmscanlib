#ifndef __INC_PALLET_CELL_H
#define __INC_PALLET_CELL_H

#include "cxtypes.h"
#include "DecodeInfo.h"

#ifdef _VISUALC_
#   include <memory>
#else
#   include <tr1/memory>
#endif

class Dib;

class PalletCell {
public:
	PalletCell(std::tr1::shared_ptr<Dib> img, unsigned row, unsigned col, int parentPosX, int parentPosY);
	~PalletCell();

	std::tr1::shared_ptr<const Dib> getImage() { return image; };

	std::tr1::shared_ptr<const CvRect> getParentPos();

	void setDecodeInfo(std::tr1::shared_ptr<const DecodeInfo> di) {
	    decodeInfo = di;
	}

	const bool getDecodeValid() {
	    return (decodeInfo.get() != NULL);
	}

	const string & getBarcodeMsg() {
	    UA_ASSERT_NOT_NULL(decodeInfo.get());
	    return decodeInfo->getMsg();
	}

	const unsigned getRow() {
	    return row;
	}

	const unsigned getCol() {
	    return col;
	}

	void getCorners(std::vector<const CvPoint *> & corners) const {
	    decodeInfo->getCorners(corners);
	}

    void getCornersInParent(std::vector<const CvPoint *> & corners) const;

private:
	std::tr1::shared_ptr<const Dib> image;
	const unsigned row;
	const unsigned col;
	CvPoint parentPos;
	std::tr1::shared_ptr<const DecodeInfo> decodeInfo;
};


#endif /* __INC_PALLET_CELL_H */
