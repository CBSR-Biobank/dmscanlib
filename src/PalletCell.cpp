#include "PalletCell.h"

PalletCell::PalletCell(std::tr1::shared_ptr<Dib> img, unsigned r, unsigned c) :
		image(img), row(r), col(c) {
	barcodePos.x = barcodePos.y = barcodePos.width = barcodePos.height = -1;
}

PalletCell::~PalletCell() {

}
