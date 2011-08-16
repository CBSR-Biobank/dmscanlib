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
