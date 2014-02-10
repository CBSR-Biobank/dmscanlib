/*
 * TestCommon.cpp
 *
 *  Created on: 2012-11-05
 *      Author: nelson
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "test/TestCommon.h"
#include "DmScanLib.h"
#include "Image.h"
#include "decoder/WellRectangle.h"
#include "decoder/DecodeOptions.h"

#include <sstream>

#include <algorithm>
#include <opencv/cv.h>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

#ifdef _VISUALC_
#   pragma warning(disable : 4996)
#   define NOMINMAX
#   include <windows.h>
#else
#   include <dirent.h>
#endif

namespace dmscanlib {

namespace test {

/*
 * Gets file names for all the test information files in the "testImageInfo" folder.
 *
 * The test images the info files refer to can be downloaded from
 * http://aicml-med.cs.ualberta.ca/CBSR/scanlib/testImages.tar.bz2
 */
bool getTestImageInfoFilenames(std::string dir, std::vector<std::string> & filenames) {
#ifndef _VISUALC_
    DIR * dp;
    dirent * dirp;

    dp = opendir(dir.c_str());
    if (dp == NULL)
        return false;

    VLOG(3) << "getting files from directory: " << dir;

    while ((dirp = readdir(dp)) != NULL) {
        if (((dirp->d_type == DT_DIR) && (dirp->d_name[0] != '.'))) {
            std::string subdirname;
            subdirname.append(dir).append("/").append(dirp->d_name);
            getTestImageInfoFilenames(subdirname, filenames);
        } else if (dirp->d_type == DT_REG) {
            std::string basename(dirp->d_name);

            if (basename.find(".nfo") != std::string::npos) {
                filenames.push_back(std::string(dir).append("/").append(basename));
            }
        }
    }
    closedir(dp);
#else
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

    //Specify a file mask. *.* = We want everything!
    std::wstring dirw;
    dirw.assign(dir.begin(), dir.end());

    std::wstring searchstrw(dirw);
    searchstrw.append(L"\\*.*");

    if((hFind = FindFirstFile((LPCWSTR) searchstrw.c_str(), &fdFile)) == INVALID_HANDLE_VALUE) {
        //VLOG(1) << "error is: " << GetLastError();
        return false;
    }

    do {
        //Find first file will always return "."
        //    and ".." as the first two directories.
        if(fdFile.cFileName[0] != '.') {

            //Is the entity a File or Folder?
            if(fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY) {
                std::wstring subdirnamew(dirw);
                subdirnamew.append(L"\\").append((wchar_t *)fdFile.cFileName);

                std::string subdirname;
                subdirname.assign(subdirnamew.begin(), subdirnamew.end());
                getTestImageInfoFilenames(subdirname, filenames);
            } else {
                std::wstring basenamew((wchar_t *)fdFile.cFileName);
                std::string basename;
                basename.assign(basenamew.begin(), basenamew.end());

                if (basename.find(".nfo") != std::string::npos) {
                    filenames.push_back(std::string(dir).append("\\").append(basename));
                }
            }
        }
    }
    while(FindNextFile(hFind, &fdFile));

    FindClose(hFind);
#endif
    return true;
}

void sbsLabelingFromRowCol(unsigned row, unsigned col, std::string & labelStr) {
    std::ostringstream label;
    label << (char) ('A' + row) << (col  + 1);
    labelStr.assign(label.str());
}

void getLabelForPosition(
    unsigned row,
    unsigned col,
    unsigned rowsMax,
    unsigned colsMax,
    Orientation orientation,
    BarcodePosition barcodePosition,
    std::string & labelStr) {

    switch (barcodePosition) {
    case TUBE_TOPS:
        switch (orientation) {
        case LANDSCAPE:
            sbsLabelingFromRowCol(row, col, labelStr);
            break;
        case PORTRAIT:
            sbsLabelingFromRowCol(colsMax - 1 - col, row, labelStr);
            break;

        default:
            throw std::logic_error("invalid value for orientation: " + orientation);
        }
        break;

    case TUBE_BOTTOMS:
        switch (orientation) {
        case LANDSCAPE:
            sbsLabelingFromRowCol(row, colsMax - 1 - col, labelStr);
            break;
        case PORTRAIT:
            sbsLabelingFromRowCol(col, row, labelStr);
            break;

        default:
            throw std::logic_error("invalid value for orientation: " + orientation);
        }
        break;

    default:
        throw std::logic_error("invalid value for barcode position: " + barcodePosition);
    }
}

void getWellRectsForBoundingBox(
        const cv::Rect & bbox,
        const unsigned rows,
        const unsigned cols,
        Orientation orientation,
        BarcodePosition position,
        std::vector<std::unique_ptr<const WellRectangle> > & wellRects) {

    float wellWidth = bbox.width / static_cast<float>(cols);
    float wellHeight = bbox.height / static_cast<float>(rows);

    cv::Point2f horTranslation(static_cast<float>(wellWidth), 0);
    cv::Point2f verTranslation(0, static_cast<float>(wellHeight));

    // round off the cell size so image dimensions are not exceeded
    cv::Size cellSize(
		static_cast<unsigned>(0.999f * wellWidth), 
		static_cast<unsigned>(0.999f * wellHeight));

    float horOffset;
    float verOffset = static_cast<float>(bbox.y);

    if ((orientation == test::LANDSCAPE) && (position == test::TUBE_BOTTOMS)) {
    } else {

    }

    for (unsigned row = 0; row < rows; ++row) {
        horOffset = static_cast<float>(bbox.x);

        for (unsigned col = 0; col < cols; ++col) {
            std::string label;
            getLabelForPosition(row, col, rows, cols, orientation, position, label);

            std::unique_ptr<WellRectangle> wellRect(
                    new WellRectangle(
                            label.c_str(),
                            static_cast<unsigned>(horOffset),
                            static_cast<unsigned>(verOffset),
                            cellSize.width,
                            cellSize.height));
            const cv::Rect & wRect = wellRect->getRectangle();
            if (!bbox.contains(wRect.tl()) || !bbox.contains(wRect.br())) {
                throw std::logic_error("well rectangle outside image: " + label);
            }

            VLOG(5) << "getWellRectsForBoundingBox: " << *wellRect;
            wellRects.push_back(std::move(wellRect));


            horOffset += wellWidth;
        }
        verOffset += wellHeight;
    }
}

std::unique_ptr<DecodeOptions> getDefaultDecodeOptions() {
    const double minEdgeFactor = 0.2;
    const double maxEdgeFactor = 0.3;
    const double scanGapFactor = 0.1;
    const long squareDev = 15;
    const long edgeThresh = 5;
    const long corrections = 10;
    const long shrink = 1;

    return std::unique_ptr<DecodeOptions>(new DecodeOptions(
            minEdgeFactor, maxEdgeFactor, scanGapFactor, squareDev, edgeThresh, corrections, shrink));
}

int decodeImage(std::string fname, DmScanLib & dmScanLib, unsigned rows, unsigned cols) {
    std::vector<std::unique_ptr<const WellRectangle> > wellRects;

    Image image(fname);
    if (!image.isValid()) {
        throw std::invalid_argument("could not load image");
    }

    cv::Size size = image.size();
    cv::Rect bbox(0, 0, size.width, size.height);

    getWellRectsForBoundingBox(bbox, rows, cols, test::LANDSCAPE, test::TUBE_BOTTOMS, wellRects);

    std::unique_ptr<DecodeOptions> decodeOptions = getDefaultDecodeOptions();
    return dmScanLib.decodeImageWells(fname.c_str(), *decodeOptions, wellRects);
}

} /* namespace test */

} /* namespace dmscanlib */
