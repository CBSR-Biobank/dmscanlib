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

void PalletCell::setDecodeInfo(DmtxDecode *dec, DmtxRegion *reg,
		DmtxMessage *msg) {
	UA_ASSERT_NOT_NULL(dec);
	UA_ASSERT_NOT_NULL(reg);
	UA_ASSERT_NOT_NULL(msg);

	DmtxVector2 p00, p10, p11, p01;

	decodedMsg.assign((char *) msg->output, msg->outputIdx);

	int height = dmtxDecodeGetProp(dec, DmtxPropHeight);
	p00.X = p00.Y = p10.Y = p01.X = 0.0;
	p10.X = p01.Y = p11.X = p11.Y = 1.0;
	dmtxMatrix3VMultiplyBy(&p00, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p10, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p11, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p01, reg->fit2raw);

	p00.Y = height - 1 - p00.Y;
	p10.Y = height - 1 - p10.Y;
	p11.Y = height - 1 - p11.Y;
	p01.Y = height - 1 - p01.Y;

	DmtxVector2 * p[4] = { &p00, &p10, &p11, &p01 };

	for (unsigned i = 0; i < 4; ++i) {
		barcodeCorners[i].x = static_cast<int>(p[i]->X);
		barcodeCorners[i].y = static_cast<int>(p[i]->Y);
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
