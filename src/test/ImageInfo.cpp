/*
 * ImageInfo.cpp
 *
 *  Created on: 2012-11-09
 *      Author: nelson
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "ImageInfo.h"

#include<iostream>
#include<fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <libconfig.h++>

namespace dmscanlib {
namespace test {

ImageInfo::ImageInfo(const std::string & fname) :
        filename(fname),
        fileValid(true),
        decodedWellCount(0) {

    libconfig::Config cfg;
    try {
      cfg.readFile(fname.c_str());
    } catch(const libconfig::FileIOException &fioex) {
      std::cerr << "I/O error while reading file" << std::endl;
      exit(EXIT_FAILURE);
    } catch(const libconfig::ParseException &pex) {
      std::cerr << "Parse error at " << pex.getFile()
              << ":" << pex.getLine()
              << " - " << pex.getError() << std::endl;
      exit(EXIT_FAILURE);
    }

    const libconfig::Setting& root = cfg.getRoot();

    if (root.lookupValue("imageFilename", imageFilename)) {
        setImageFilename(imageFilename);
    } else {
        std::cerr << "setting not found in configuration file: image file name" << std::endl;
        fileValid = false;
    }

    try {
        const libconfig::Setting& bbox = root["boundingBox"];

        unsigned x, y, width, height;

        if (bbox.lookupValue("x", x)
                && bbox.lookupValue("y", y)
                && bbox.lookupValue("width", width)
                && bbox.lookupValue("height", height)) {
            boundingBox = std::unique_ptr<const cv::Rect>(new cv::Rect(x, y, width, height));
        } else {
            std::cerr << "invalid setting in configuration file: bounding box" << std::endl;
            fileValid = false;
        }
    } catch(const libconfig::SettingNotFoundException &nfex) {
        std::cerr << "setting not found in configuration file: image file name" << std::endl;
        fileValid = false;
    }

    try {
        const libconfig::Setting& bbox = root["palletSize"];

        if (!bbox.lookupValue("rows", palletRows)
                || !bbox.lookupValue("columns", palletCols)) {
            std::cerr << "invalid setting in configuration file: bounding box" << std::endl;
            fileValid = false;
        }
    } catch(const libconfig::SettingNotFoundException &nfex) {
        std::cerr << "setting not found in configuration file: bounding box" << std::endl;
        fileValid = false;
    }

    std::string orientationStr;
    if (root.lookupValue("orientation", orientationStr)) {
        orientation = DmScanLib::getOrientationFromString(orientationStr);
    } else {
        std::cerr << "setting not found in configuration file: orientation" << std::endl;
        fileValid = false;
    }

    std::string barcodePositionStr;
    if (root.lookupValue("barcodePosition", barcodePositionStr)) {
        barcodePosition = DmScanLib::getBarcodePositionFromString(barcodePositionStr);
    } else {
        std::cerr << "setting not found in configuration file: barcodePosition" << std::endl;
        fileValid = false;
    }

    for (unsigned row = 0; row < palletRows; ++row) {
        for (unsigned col = 0; col < palletCols; ++col) {
            std::string label, decodedMessage;
            DmScanLib::getLabelForPosition(
                    row,
                    col,
                    palletRows,
                    palletCols,
                    orientation,
                    barcodePosition,
                    label);

            if (root.lookupValue(label, decodedMessage)) {
                barcodePosition = DmScanLib::getBarcodePositionFromString(barcodePositionStr);
                wells.insert(std::make_pair(label, decodedMessage));
            } else {
                std::cerr << "setting not found in configuration file: " << label << std::endl;
                fileValid = false;
            }
        }
    }
}

void ImageInfo::setImageFilename(std::string & basename) {
    imageFilename = basename;
    std::ifstream file;
    file.open(imageFilename);
    imageFileValid = file.good();
    file.close();
}

std::vector<std::string> & ImageInfo::split(const std::string &s, char delim,
        std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

unsigned ImageInfo::stringToUnsigned(const std::string& s) {
    std::istringstream i(s);
    unsigned x;
    if (!(i >> x))
        return 0;
    return x;
}

const std::string * ImageInfo::getBarcodeMsg(const std::string & label) {
    if (wells.find(label) == wells.end()) {
        return NULL;
    }

    return &wells[label];
}

std::ostream & operator<<(std::ostream &os, const ImageInfo & m) {
    os << "imageFilename=\"" << m.imageFilename << "\"" << std::endl;

    os << "boundingBox = {"
            << " x = " << m.boundingBox->x
            << "; y = " << m.boundingBox->y
            << "; width = " << m.boundingBox->width
            << "; height = " << m.boundingBox->height
            << "; }"
            << std::endl;

    os << "palletSize = {"
            << " rows = "<< m.palletRows
            << "; columns=" << m.palletCols
            << "; }"
            << std::endl;

    os << "orientation=\"" << m.orientation << "\"" << std::endl;
    os << "barcodePosition=\"" << m.barcodePosition << "\"" << std::endl;

    std::map<const std::string, const std::string>::const_iterator ii = m.wells.begin();

    for (; ii != m.wells.end(); ++ii) {
        os << ii->first << "=\"" << ii->second << "\"" << std::endl;
    }
    return os;
}

} /* namespace test */

} /* namespace dmscanlib */
