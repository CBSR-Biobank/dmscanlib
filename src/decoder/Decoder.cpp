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

#include <glog/logging.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

namespace dmscanlib {

Decoder::Decoder(const Dib & _image, const DecodeOptions & _decodeOptions,
		std::vector<std::unique_ptr<WellRectangle<double>  > > & _wellRects) :
		image(_image), decodeOptions(_decodeOptions), wellRects(_wellRects)
{
	wellDecoders.resize(wellRects.size());
	applyFilters();
}

Decoder::~Decoder() {
}

int Decoder::decodeWellRects() {
	const unsigned dpi = image.getDpi();

	VLOG(3) << "DecodeCommon: dpi/" << dpi << " numWellRects/" << wellRects.size();

	if ((dpi != 300) && (dpi != 400) && (dpi != 600)) {
		return SC_INCORRECT_DPI_SCANNED;
	}

	for(unsigned i = 0, n = wellRects.size(); i < n; ++i) {
		const WellRectangle<double> & wellRect = *wellRects[i];

		VLOG(3) << "well rect: " << wellRect;

		std::unique_ptr<const Rect<double> > factoredRect = std::move(
				wellRect.getRectangle().scale(static_cast<double>(dpi)));

		std::unique_ptr<WellRectangle<unsigned> > convertedWellTect(
				new WellRectangle<unsigned>(wellRect.getLabel().c_str(),
						static_cast<unsigned>(factoredRect->corners[0].x),
						static_cast<unsigned>(factoredRect->corners[0].y),
						static_cast<unsigned>(factoredRect->corners[1].x),
						static_cast<unsigned>(factoredRect->corners[1].y),
						static_cast<unsigned>(factoredRect->corners[2].x),
						static_cast<unsigned>(factoredRect->corners[2].y),
						static_cast<unsigned>(factoredRect->corners[3].x),
						static_cast<unsigned>(factoredRect->corners[3].y)));

		wellDecoders[i] = std::unique_ptr<WellDecoder>(
				new WellDecoder(*this, std::move(convertedWellTect)));
	}

	decoder::ThreadMgr threadMgr;
	threadMgr.decodeWells(wellDecoders);

	for(unsigned i = 0, n = wellDecoders.size(); i < n; ++i) {
		WellDecoder & wellDecoder = *wellDecoders[i];
		VLOG(2) << wellDecoder;
		if (!wellDecoder.getMessage().empty()) {
			++decodedWellCount;
			decodedWells.push_back(&wellDecoder);
		}
	}
	return SC_SUCCESS;
}

void Decoder::applyFilters() {
	filteredImage = (image.getBitsPerPixel() != 8)
			? std::move(image.convertGrayscale())
			: std::unique_ptr<Dib>(new Dib(image));

	filteredImage->tpPresetFilter();
	if (VLOG_IS_ON(2)) {
		filteredImage->writeToFile("filtered.bmp");
	}
}

/*
 * Called by multiple threads.
 */
void Decoder::decodeWellRect(const Dib & wellRectImage, WellDecoder & wellDecoder) const {
	const unsigned dpi = wellRectImage.getDpi();
	CHECK((dpi == 300) || (dpi == 400) || (dpi == 600));

	DmtxImage * dmtxImage = wellRectImage.getDmtxImage();
	DmtxDecode *dec = dmtxDecodeCreate(dmtxImage, 1);

	CHECK_NOTNULL(dmtxImage);
	CHECK_NOTNULL(dec);

	// slightly smaller than the new tube edge
	dmtxDecodeSetProp(dec, DmtxPropEdgeMin, static_cast<int>(0.08 * dpi));

	// slightly bigger than the Nunc edge
	dmtxDecodeSetProp(dec, DmtxPropEdgeMax, static_cast<int>(0.18 * dpi));

	dmtxDecodeSetProp(dec, DmtxPropSymbolSize, DmtxSymbolSquareAuto);
	dmtxDecodeSetProp(dec, DmtxPropScanGap,
			static_cast<unsigned>(decodeOptions.scanGap * dpi));
	dmtxDecodeSetProp(dec, DmtxPropSquareDevn, decodeOptions.squareDev);
	dmtxDecodeSetProp(dec, DmtxPropEdgeThresh, decodeOptions.edgeThresh);

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

	dmtxDecodeDestroy(&dec);
	dmtxImageDestroy(&dmtxImage);
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

	Rect<unsigned> decodeRect(p00.X, p00.Y, p10.X, p10.Y, p11.X, p11.Y, p01.X, p01.Y);
	wellDecoder.setDecodeRectangle(decodeRect);
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
