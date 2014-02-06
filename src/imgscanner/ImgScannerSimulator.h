#ifndef __INCLUDE_IMG_SCANNER_SIMULATOR_H
#define __INCLUDE_IMG_SCANNER_SIMULATOR_H

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

#include "imgscanner/ImgScanner.h"

namespace dmscanlib {

namespace imgscanner {

/**
 * This class interfaces with the TWAIN driver to acquire images from the
 * scanner.
 */
class ImgScannerSimulator: public ImgScanner {
public:
    ImgScannerSimulator();
    virtual ~ImgScannerSimulator();

    bool selectSourceAsDefault();

    int getScannerCapability();

    HANDLE acquireImage(
            const unsigned dpi,
            const int brightness,
            const int contrast,
            const cv::Rect_<float> & bbox);

    HANDLE acquireFlatbed(
            const unsigned dpi,
            const int brightness,
            const int contrast);

    void freeImage(HANDLE handle);

    int getErrorCode() {
        return 0;
    }

private:
};

} /* namespace */

} /* namespace */

#endif /* __INCLUDE_IMG_SCANNER_SIMULATOR_H */
