/*
 Dmscanlib is a software library and standalone application that scans
 and decodes libdmtx compatible test-tubes. It is currently designed
 to decode 12x8 pallets that use 2D data-matrix laser etched test-tubes.
 Copyright (C) 2010 Canadian Biosample Repository

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef _VISUALC_
// disable fopen warnings
#pragma warning(disable : 4996)
#endif

#include "Decoder.h"
#include "DmScanLib.h"
#include "DecodeOptions.h"
#include "dib/Dib.h"
#include "WellDecoder.h"
#include "decoder/ThreadMgr.h"
#include "decoder/DmtxDecodeHelper.h"

#include <glog/logging.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sstream>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

namespace dmscanlib {

using namespace decoder;

Decoder::Decoder(const Dib & _image, const DecodeOptions & _decodeOptions,
		std::vector<std::unique_ptr<WellRectangle<unsigned>  > > & _wellRects) :
		image(_image), dmtxImage(NULL), dpi(image.getDpi()), decodeOptions(_decodeOptions),
		wellRects(_wellRects), decodeSuccessful(false)
{
	CHECK((dpi == 300) || (dpi == 400) || (dpi == 600));

	unsigned width = image.getWidth();
	unsigned height = image.getHeight();

	for(unsigned i = 0, n = wellRects.size(); i < n; ++i) {
		// ensure well rectangles are within the image's region
		const WellRectangle<unsigned> & wellRect = *wellRects[i];

		std::unique_ptr<const BoundingBox<unsigned> > wellBbox =
				wellRect.getRectangle().getBoundingBox();

		if ((wellBbox->points[0].x >= width)
				|| (wellBbox->points[0].y >= height)
				|| (wellBbox->points[1].x >= width)
				|| (wellBbox->points[1].y >= height)) {
			throw std::invalid_argument("well rectangle exeeds image dimensions: "
					+ wellRect.getLabel());
		}
	}

	wellDecoders.resize(wellRects.size());
	applyFilters();

	CHECK_NOTNULL(filteredImage.get());
	dmtxImage = filteredImage->getDmtxImage();
}

Decoder::~Decoder() {
	dmtxImageDestroy(&dmtxImage);
}

void Decoder::applyFilters() {
	CHECK(dmtxImage == NULL);

	filteredImage = (image.getBitsPerPixel() != 8)
			? std::move(image.convertGrayscale())
			: std::unique_ptr<Dib>(new Dib(image));

	filteredImage->tpPresetFilter();
	if (VLOG_IS_ON(2)) {
		filteredImage->writeToFile("filtered.bmp");
	}
}

int Decoder::decodeWellRects() {
	if ((dpi != 300) && (dpi != 400) && (dpi != 600)) {
		return SC_INCORRECT_DPI_SCANNED;
	}

	VLOG(3) << "decodeWellRects: dpi/" << dpi << " numWellRects/" << wellRects.size();

	for(unsigned i = 0, n = wellRects.size(); i < n; ++i) {
		const WellRectangle<unsigned> & wellRect = *wellRects[i];

		VLOG(5) << "well rect: " << wellRect;

		wellDecoders[i] = std::unique_ptr<WellDecoder>(
				new WellDecoder(*this, wellRect));
	}
	return decodeMultiThreaded();
	//return decodeSingleThreaded();

}

int Decoder::decodeSingleThreaded() {
	for(unsigned i = 0, n = wellDecoders.size(); i < n; ++i) {
		wellDecoders[i]->run();
		if (!wellDecoders[i]->getMessage().empty()) {
			decodedWells[wellDecoders[i]->getMessage()] = wellDecoders[i].get();
		}
	}
	return SC_SUCCESS;
}

int Decoder::decodeMultiThreaded() {
	decoder::ThreadMgr threadMgr;
	threadMgr.decodeWells(wellDecoders);

	for(unsigned i = 0, n = wellDecoders.size(); i < n; ++i) {
		WellDecoder & wellDecoder = *wellDecoders[i];
		VLOG(2) << wellDecoder;
		if (!wellDecoder.getMessage().empty()) {
			if (decodedWells.find(wellDecoder.getMessage()) != decodedWells.end()) {
				return SC_FAIL;
			}

			decodedWells[wellDecoder.getMessage()] = &wellDecoder;
		}
	}
	decodeSuccessful = true;
	return SC_SUCCESS;
}

const unsigned Decoder::getDecodedWellCount() {
	if (!decodeSuccessful) return 0;

	return decodedWells.size();
}

const std::map<std::string, const WellDecoder *> & Decoder::getDecodedWells() const {
	if (!decodeSuccessful) {
		throw std::logic_error("duplicate decoded messages found");
	}
	return decodedWells;
}

/*
 * Called by multiple threads.
 */
void Decoder::decodeWellRect(WellDecoder & wellDecoder) const {
	CHECK_NOTNULL(dmtxImage);

	VLOG(2) << "decodeWellRect: " << wellDecoder;

	std::unique_ptr<DmtxDecodeHelper> dec = createDmtxDecode(wellDecoder, decodeOptions.shrink);
	decodeWellRect(wellDecoder, dec->getDecode());

	if (wellDecoder.getMessage().empty()) {
		VLOG(2) << "decodeWellRect: second attempt " << wellDecoder;
		dec = std::move(createDmtxDecode(wellDecoder, decodeOptions.shrink + 1));
		decodeWellRect(wellDecoder, dec->getDecode());
	}
}

std::unique_ptr<DmtxDecodeHelper> Decoder::createDmtxDecode(
		WellDecoder & wellDecoder, int scale) const {
	std::unique_ptr<DmtxDecodeHelper> dec(new DmtxDecodeHelper(dmtxImage, scale));

	unsigned height = image.getHeight();

	std::unique_ptr<const BoundingBox<unsigned> > bbox = std::move(
			wellDecoder.getWellRectangle().getBoundingBox());

	dec->setProperty(DmtxPropXmin, bbox->points[0].x);
	dec->setProperty(DmtxPropXmax, bbox->points[1].x);

	dec->setProperty(DmtxPropYmax, height - bbox->points[0].y);
	dec->setProperty(DmtxPropYmin, height - bbox->points[1].y);


	// slightly smaller than the new tube edge
	dec->setProperty(DmtxPropEdgeMin, static_cast<int>(0.08 * dpi));

	// slightly bigger than the Nunc edge
	dec->setProperty(DmtxPropEdgeMax, static_cast<int>(0.18 * dpi));

	dec->setProperty(DmtxPropSymbolSize, DmtxSymbolSquareAuto);
	dec->setProperty(DmtxPropScanGap,
			static_cast<unsigned>(decodeOptions.scanGap * dpi));
	dec->setProperty(DmtxPropSquareDevn, decodeOptions.squareDev);
	dec->setProperty(DmtxPropEdgeThresh, decodeOptions.edgeThresh);

	return dec;
}

void Decoder::decodeWellRect(WellDecoder & wellDecoder, DmtxDecode *dec) const {
	DmtxRegion * reg;
	while (1) {
		reg = dmtxRegionFindNext(dec, NULL);
		if (reg == NULL) {
			break;
		}

		DmtxMessage *msg = dmtxDecodeMatrixRegion(dec, reg, decodeOptions.corrections);
		if (msg != NULL) {
			getDecodeInfo(dec, reg, msg, wellDecoder);

			if (VLOG_IS_ON(5)) {
				showStats(dec, reg, msg);
			}
			dmtxMessageDestroy(&msg);
		}
		dmtxRegionDestroy(&reg);
	}

	if (VLOG_IS_ON(5)) {
		writeDiagnosticImage(dec, wellDecoder.getLabel());
	}

}

void Decoder::getDecodeInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg,
		WellDecoder & wellDecoder) const {
	CHECK_NOTNULL(dec);
	CHECK_NOTNULL(reg);
	CHECK_NOTNULL(msg);

	DmtxVector2 p00, p10, p11, p01;

	wellDecoder.setMessage((char *) msg->output, msg->outputIdx);

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

	Rect<double> decodeRect(p00.X, p00.Y, p10.X, p10.Y, p11.X, p11.Y, p01.X, p01.Y);
	wellDecoder.setDecodeRectangle(decodeRect, dec->scale);
}

void Decoder::showStats(DmtxDecode * dec, DmtxRegion * reg, DmtxMessage * msg) const {
	if (!VLOG_IS_ON(5)) return;

	int height;
	int dataWordLength;
	int rotateInt;
	double rotate;
	DmtxVector2 p00, p10, p11, p01;

	height = dmtxDecodeGetProp(dec, DmtxPropHeight);

	p00.X = p00.Y = p10.Y = p01.X = 0.0;
	p10.X = p01.Y = p11.X = p11.Y = 1.0;
	dmtxMatrix3VMultiplyBy(&p00, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p10, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p11, reg->fit2raw);
	dmtxMatrix3VMultiplyBy(&p01, reg->fit2raw);

	dataWordLength = dmtxGetSymbolAttribute(DmtxSymAttribSymbolDataWords,
			reg->sizeIdx);

	rotate = (2 * M_PI)
			+ (atan2(reg->fit2raw[0][1], reg->fit2raw[1][1])
					- atan2(reg->fit2raw[1][0], reg->fit2raw[0][0])) / 2.0;

	rotateInt = (int) (rotate * 180 / M_PI + 0.5);
	if (rotateInt >= 360)
		rotateInt -= 360;

	VLOG(5) << "\n--------------------------------------------------"
			<< "\n       Matrix Size: "
			<< dmtxGetSymbolAttribute(DmtxSymAttribSymbolRows, reg->sizeIdx)
			<< " x "
			<< dmtxGetSymbolAttribute(DmtxSymAttribSymbolCols, reg->sizeIdx)
			<< "\n    Data Codewords: "
			<< dataWordLength - msg->padCount << " (capacity "
			<< dataWordLength << ")" << "\n   Error Codewords: "
			<< dmtxGetSymbolAttribute(DmtxSymAttribSymbolErrorWords, reg->sizeIdx)
			<< "\n      Data Regions: "
			<< dmtxGetSymbolAttribute(DmtxSymAttribHorizDataRegions, reg->sizeIdx)
			<< " x "
			<< dmtxGetSymbolAttribute(DmtxSymAttribVertDataRegions, reg->sizeIdx)
			<< "\nInterleaved Blocks: "
			<< dmtxGetSymbolAttribute(DmtxSymAttribInterleavedBlocks, reg->sizeIdx)
			<< "\n    Rotation Angle: "
			<< rotateInt << "\n          Corner 0: (" << p00.X << ", "
			<< height - 1 - p00.Y << ")" << "\n          Corner 1: ("
			<< p10.X << ", " << height - 1 - p10.Y << ")"
			<< "\n          Corner 2: (" << p11.X << ", "
			<< height - 1 - p11.Y << ")" << "\n          Corner 3: ("
			<< p01.X << ", " << height - 1 - p01.Y << ")"
			<< "\n--------------------------------------------------";
}

void Decoder::writeDiagnosticImage(DmtxDecode *dec, const std::string & id) const {
	if (!VLOG_IS_ON(5))
		return;

	int totalBytes, headerBytes;
	int bytesWritten;
	unsigned char *pnm;
	FILE *fp;

	std::ostringstream fname;
	fname << "diagnostic-" << id << ".pnm";

	fp = fopen(fname.str().c_str(), "wb");
	CHECK_NOTNULL(fp);

	pnm = dmtxDecodeCreateDiagnostic(dec, &totalBytes, &headerBytes, 0);
	CHECK_NOTNULL(pnm);

	bytesWritten = fwrite(pnm, sizeof(unsigned char), totalBytes, fp);
	CHECK(bytesWritten == totalBytes);

	free(pnm);
	fclose(fp);
}

} /* namespace */
