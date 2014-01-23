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

// disable fopen warnings
#define _CRT_SECURE_NO_DEPRECATE

#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellDecoder.h"
#include "decoder/ThreadMgr.h"
#include "decoder/DmtxDecodeHelper.h"
#include "Image.h"
#include "DmScanLib.h"

#include <glog/logging.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sstream>
#include <opencv/cv.h>

#if defined(USE_NVWA)
#   include "debug_new.h"
#endif

namespace dmscanlib {

using namespace decoder;

Decoder::Decoder(
        const Image & image,
        const DecodeOptions & _decodeOptions,
        std::vector<std::unique_ptr<const WellRectangle> > & _wellRects) :
        decodeOptions(_decodeOptions),
        wellRects(_wellRects),
        decodeSuccessful(false)
{
//    grayscaleImage = image.grayscale();
    std::unique_ptr<const Image> filteredImage = image.applyFilters();
        if (VLOG_IS_ON(2)) {
            filteredImage->write("filtered.png");
        }
    grayscaleImage = filteredImage->grayscale();

    cv::Size size = grayscaleImage->size();
    cv::Rect imageRect(0, 0, size.width, size.height);
    float width = static_cast<float>(size.width);
    float height = static_cast<float>(size.height);

    VLOG(5) << "Decoder: image size: " << width << ", " << height;

    for (unsigned i = 0, n = wellRects.size(); i < n; ++i) {
        // ensure well rectangles are within the image's region
        const WellRectangle & wellRect = *wellRects[i];

        const cv::Rect & rect = wellRect.getRectangle();

        VLOG(5) << "Decoder: well: " << rect;

        if (!imageRect.contains(rect.tl()) || !imageRect.contains(rect.br())) {
            throw std::invalid_argument("well rectangle exceeds image dimensions: "
                    + wellRect.getLabel());
        }
    }

    wellDecoders.resize(wellRects.size());
}

Decoder::~Decoder() {
}

int Decoder::decodeWellRects() {
    VLOG(3) << "decodeWellRects: numWellRects/" << wellRects.size();

    for (unsigned i = 0, n = wellRects.size(); i < n; ++i) {
        const WellRectangle & wellRect = *wellRects[i];

        VLOG(5) << "well rect: " << wellRect;

        const cv::Rect & rect = wellRect.getRectangle();

        std::unique_ptr<WellRectangle> convertedWellRect(
                new WellRectangle(
                        wellRect.getLabel().c_str(),
                        rect.x,
                        rect.y,
                        rect.width,
                        rect.height
                ));

        wellDecoders[i] = std::unique_ptr<WellDecoder>(
                new WellDecoder(*this, std::move(convertedWellRect)));
    }
    return decodeMultiThreaded();
    //return decodeSingleThreaded();
}

int Decoder::decodeSingleThreaded() {
    for (unsigned i = 0, n = wellDecoders.size(); i < n; ++i) {
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

    for (unsigned i = 0, n = wellDecoders.size(); i < n; ++i) {
        WellDecoder & wellDecoder = *wellDecoders[i];
        VLOG(5) << wellDecoder;
        if (!wellDecoder.getMessage().empty()) {
            if (decodedWells.find(wellDecoder.getMessage()) != decodedWells.end()) {
                VLOG(1) << "duplicate decode message found: " << wellDecoder.getMessage();
                return SC_FAIL;
            }

            decodedWells[wellDecoder.getMessage()] = &wellDecoder;
        }
    }
    decodeSuccessful = true;
    return SC_SUCCESS;
}

const unsigned Decoder::getDecodedWellCount() {
    if (!decodeSuccessful)
        return 0;

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
void Decoder::decodeWellRect(const Image & wellRectImage, WellDecoder & wellDecoder) const {
    DmtxImage * dmtxImage = wellRectImage.dmtxImage();
    CHECK_NOTNULL(dmtxImage);

    std::unique_ptr<DmtxDecodeHelper> dec =
            createDmtxDecode(dmtxImage, wellDecoder, decodeOptions.shrink);
    decodeWellRect(wellDecoder, dec->getDecode());
    VLOG(5) << "decodeWellRect: " << wellDecoder;

    if (wellDecoder.getMessage().empty()) {
        dec = std::move(createDmtxDecode(
                dmtxImage, wellDecoder, decodeOptions.shrink + 1));
        decodeWellRect(wellDecoder, dec->getDecode());
        VLOG(5) << "decodeWellRect: second attempt " << wellDecoder;
    }
    dmtxImageDestroy(&dmtxImage);
}

std::unique_ptr<DmtxDecodeHelper> Decoder::createDmtxDecode(
        DmtxImage * dmtxImage,
        WellDecoder & wellDecoder,
        int scale) const {
    std::unique_ptr<DmtxDecodeHelper> dec(new DmtxDecodeHelper(dmtxImage, scale));

    cv::Rect bbox = wellDecoder.getWellRectangle();

    unsigned mindim = std::min(bbox.width, bbox.height);

    dec->setProperty(DmtxPropEdgeMin, static_cast<int>(decodeOptions.minEdgeFactor * mindim));
    dec->setProperty(DmtxPropEdgeMax, static_cast<int>(decodeOptions.maxEdgeFactor * mindim));
    dec->setProperty(DmtxPropScanGap, static_cast<int>(decodeOptions.scanGapFactor * mindim));

    dec->setProperty(DmtxPropSymbolSize, DmtxSymbolSquareAuto);
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

void Decoder::getDecodeInfo(
        DmtxDecode *dec,
        DmtxRegion *reg,
        DmtxMessage *msg,
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

    cv::Point2f points[4] = {
            cv::Point2f(p00.X, p00.Y) * dec->scale,
            cv::Point2f(p10.X, p10.Y) * dec->scale,
            cv::Point2f(p11.X, p11.Y) * dec->scale,
            cv::Point2f(p01.X, p01.Y) * dec->scale
    };

    wellDecoder.setDecodeQuad(points);
}

void Decoder::showStats(DmtxDecode * dec, DmtxRegion * reg, DmtxMessage * msg) {
    if (!VLOG_IS_ON(5))
        return;

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

void Decoder::writeDiagnosticImage(DmtxDecode *dec, const std::string & id) {
    //if (!VLOG_IS_ON(5)) return;

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
