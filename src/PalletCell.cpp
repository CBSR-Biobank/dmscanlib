#include "PalletCell.h"
#include "Dib.h"
#include "UaAssert.h"

// TODO: make sure no two barcode messages are the same

PalletCell::PalletCell(std::tr1::shared_ptr<Dib> img, unsigned r, unsigned c,
		CvRect & pos) :
		image(img), row(r), col(c), parentRect(pos) {
}

PalletCell::~PalletCell() {

}

std::tr1::shared_ptr<const CvRect> PalletCell::getParentPos() {
	CvRect r;
	r.x = parentRect.x;
	r.y = parentRect.y;
	r.width = image->getWidth();
	r.height = image->getHeight();
	return std::tr1::shared_ptr<const CvRect>(new CvRect(r));
}

const string & PalletCell::getBarcodeMsg() {
    UA_ASSERT(!decodedMsg.empty());
    return decodedMsg;
}

void PalletCell::setDecodeInfo(std::string & msg, CvPoint(&corners)[4]) {
	decodedMsg = msg;

	for (unsigned i = 0; i < 4; ++i) {
		barcodeCorners[i] = corners[i];
	}
}

void PalletCell::drawCellBox(Dib & image, const RgbQuad & color) const {
	image.rectangle(parentRect.x, parentRect.y, parentRect.width, parentRect.height,
			color);
}

void PalletCell::drawBarcodeBox(Dib & image, const RgbQuad & color) const {
	CvPoint corners[4];

	for (unsigned i = 0; i < 4; ++i) {
		corners[i].x = barcodeCorners[i].x + parentRect.x;
		corners[i].y = barcodeCorners[i].y + parentRect.y;
	}

	image.line(corners[0], corners[1], color);
	image.line(corners[1], corners[2], color);
	image.line(corners[2], corners[3], color);
	image.line(corners[3], corners[0], color);
}

ostream & operator<<(ostream &os, PalletCell & m) {
	os << "\"" << m.decodedMsg << "\" (" << m.barcodeCorners[0].x << ", "
			<< m.barcodeCorners[0].y << "), " << "(" << m.barcodeCorners[2].x
			<< ", " << m.barcodeCorners[2].y << "), " << "("
			<< m.barcodeCorners[3].x << ", " << m.barcodeCorners[3].y << "), "
			<< "(" << m.barcodeCorners[1].x << ", " << m.barcodeCorners[1].y
			<< ")";
	return os;
}
