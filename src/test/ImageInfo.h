/*
 * ImageInfo.h
 *
 *  Created on: 2012-11-09
 *      Author: nelson
 */

#ifndef IMAGEINFO_H_
#define IMAGEINFO_H_

#include "decoder/WellRectangle.h"
#include "DmScanLib.h"

#include <string>
#include <map>
#include <memory>

namespace dmscanlib {

namespace test {

class ImageInfo {
public:
    ImageInfo(const std::string & filename);
    virtual ~ImageInfo() {
    }

    const bool isValid() {
        return fileValid && imageFileValid;
    }

    const std::string & getImageFilename() const {
        return imageFilename;
    }

    const std::string * getBarcodeMsg(const std::string & label);

    const cv::Rect & getBoundingBox() const {
        return *boundingBox;
    }

    const Orientation getOrientation() const {
        return orientation;
    }

    const BarcodePosition getBarcodePosition() const {
        return barcodePosition;
    }

    const unsigned getDecodedWellCount() const {
        return decodedWellCount;
    }

    const unsigned getPalletRows() const {
        return palletRows;
    }

    const unsigned getPalletCols() const {
        return palletCols;
    }

private:
    std::vector<std::string> & split(
            const std::string &s, char delim,
            std::vector<std::string> &elems);
    unsigned stringToUnsigned(const std::string& s);
    void toCout();
    void setImageFilename(std::string & basename);

    std::string filename;
    bool fileValid;
    bool imageFileValid;
    std::string imageFilename;
    std::unique_ptr<const cv::Rect> boundingBox;
    Orientation orientation;
    BarcodePosition barcodePosition;
    unsigned palletRows;
    unsigned palletCols;
    std::map<const std::string, const std::string> wells;
    unsigned decodedWellCount;

    friend std::ostream & operator<<(std::ostream & os, const ImageInfo & m);
};

} /* namespace test */
} /* namespace dmscanlib */

#endif /* IMAGEINFO_H_ */
