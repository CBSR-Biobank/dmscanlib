#ifndef __INC_SCANLIB_INTERNAL_H
#define __INC_SCANLIB_INTERNAL_H

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

#include "decoder/WellRectangle.h"
#include "utils/DmTime.h"

#include <string>
#include <memory>
#include <vector>
#include <map>

namespace dmscanlib {

/**
 * Return codes used by the DLL API.
 */
const int SC_SUCCESS = 0;
const int SC_FAIL = -1;
const int SC_TWAIN_UNAVAIL = -2;
const int SC_INVALID_DPI = -3;
const int SC_INVALID_NOTHING_DECODED = -4;
const int SC_INVALID_IMAGE = -5;
const int SC_INVALID_NOTHING_TO_DECODE = -6;
const int SC_INCORRECT_DPI_SCANNED = -7;

const unsigned CAP_IS_WIA = 0x01;
const unsigned CAP_DPI_300 = 0x02;
const unsigned CAP_DPI_400 = 0x04;
const unsigned CAP_DPI_600 = 0x08;
const unsigned CAP_IS_SCANNER = 0x10;

class Image;
class Decoder;
class ImgScanner;
class WellDecoder;
class DecodeOptions;

enum Orientation { LANDSCAPE, PORTRAIT, ORIENTATION_MAX };

enum BarcodePosition { TUBE_TOPS, TUBE_BOTTOMS, BARCODE_POSITION_MAX };

enum PalletSize { PSIZE_8x12, PSIZE_10x10, PSIZE_1x1, PSIZE_12x12, PSIZE_9x9, PSIZE_MAX };

class DmScanLib {
public:
    DmScanLib();
    DmScanLib(unsigned loggingLevel, bool logToFile = true);
    virtual ~DmScanLib();

    int selectSourceAsDefault();

    int getScannerCapability();

	/**
	* (x1, y1) is the top left corner
	* (x2, y2) is the bottom right corner
	*/
    int scanImage(
            const unsigned dpi,
            const int brightness,
            const int contrast,
            const float x,
            const float y,
            const float width,
            const float height,
            const char * filename);

    int scanFlatbed(
            unsigned dpi,
            int brightness,
            int contrast,
            const char * filename);

    int scanAndDecode(
            const unsigned dpi,
            const int brightness,
            const int contrast,
            const float x,
            const float y,
            const float width,
            const float height,
            const DecodeOptions & decodeOptions,
            std::vector<std::unique_ptr<const WellRectangle> > & wellRects);

    int decodeImageWells(
            const char * filename,
            const DecodeOptions & decodeOptions,
            std::vector<std::unique_ptr<const WellRectangle> > & wellRects);

    static void configLogging(unsigned level, bool useFile = true);

    const unsigned getDecodedWellCount();

    const std::map<std::string, const WellDecoder *> & getDecodedWells() const;


    static Orientation getOrientationFromString(std::string & orientationStr);

    static BarcodePosition getBarcodePositionFromString(std::string & positionStr);

    static void sbsLabelingFromRowCol(unsigned row, unsigned col, std::string & labelStr);

    static void getLabelForPosition(
        unsigned row,
        unsigned col,
        unsigned rowsMax,
        unsigned colsMax,
        Orientation orientation,
        BarcodePosition barcodePosition,
        std::string & labelStr);

    static PalletSize getPalletSizeFromString(std::string & palletSizeStr);

protected:
    int decodeCommon(
            const Image & image,
            const DecodeOptions & decodeOptions,
            const std::string &decodedDibFilename,
            std::vector<std::unique_ptr<const WellRectangle> > & wellRects);

    void writeDecodedImage(const Image & image, const std::string & decodedDibFilename);

    static const std::string LIBRARY_NAME;

    std::unique_ptr<ImgScanner> imgScanner;

    std::unique_ptr<Decoder> decoder;

    static bool loggingInitialized;

};

} /* namespace */

#endif /* __INC_SCANLIB_INTERNAL_H */
