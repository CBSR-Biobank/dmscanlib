#include "PalletCell.h"
#include "Dib.h"

PalletCell::PalletCell(std::tr1::shared_ptr<Dib> img, unsigned r, unsigned c, int x, int y) :
		image(img), row(r), col(c) {
	parentPos.x = x;
	parentPos.y = y;
}

PalletCell::~PalletCell() {

}

std::tr1::shared_ptr<const CvRect> PalletCell::getParentPos() {
	CvRect r;
	r.x = parentPos.x;
	r.y = parentPos.y;
	r.width = image->getWidth();
	r.height = image->getHeight();
	return std::tr1::shared_ptr<const CvRect>(new CvRect(r));
}

void PalletCell::getCornersInParent(std::vector<const CvPoint *> & result) const {
    std::vector<const CvPoint *> barcodeCorners;
    getCorners(barcodeCorners);

    for (unsigned i = 0; i < 4; ++i) {
        result[i]->x = barcodeCorners[i]->x + parentPos.x;
        result[i]->y = barcodeCorners[i]->y + parentPos.y;
    }
}
