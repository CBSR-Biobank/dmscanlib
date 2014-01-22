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

void getWellRectsForBoundingBox(
        const BoundingBox<unsigned> & bbox,
        const unsigned rows,
        const unsigned cols,
        std::vector<std::unique_ptr<const WellRectangle> > & wellRects) {

    double wellWidth = bbox.getWidth() / static_cast<float>(cols);
    double wellHeight = bbox.getHeight() / static_cast<float>(rows);

    cv::Point_<float> startPoint(
            static_cast<float>(bbox.points[0].x),
            static_cast<float>(bbox.points[0].y));
    cv::Point2f horTranslation(static_cast<float>(wellWidth), 0);
    cv::Point2f verTranslation(0, static_cast<float>(wellHeight));

    // round off the bounding box so image dimensions are not exceeded
    cv::Size2f pt2(0.999 * wellWidth, 0.999 * wellHeight);

    float verOffset = 0;
    float horOffset = 0;

    for (unsigned row = 0; row < rows; ++row) {
        horOffset = 0;

        for (unsigned col = 0; col < cols; ++col) {

            std::ostringstream label;
            label << (char) ('A' + row) << cols - col;

            std::unique_ptr<WellRectangle> wellRect(
                    new WellRectangle(
                            label.str().c_str(),
                            horOffset,
                            verOffset,
                            pt2.width,
                            pt2.height));
            VLOG(3) << *wellRect;
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

    cv::Point_<unsigned> pt1(0, 0);
    cv::Point_<unsigned> pt2(size.width, size.height);
    BoundingBox<unsigned> bbox(pt1, pt2);

    getWellRectsForBoundingBox(bbox, rows, cols, wellRects);

    std::unique_ptr<DecodeOptions> decodeOptions = getDefaultDecodeOptions();
    return dmScanLib.decodeImageWells(fname.c_str(), *decodeOptions, wellRects);
}

// bbox here has to start at (0,0)
std::unique_ptr<const BoundingBox<float>> getWellsBoundingBox(
        const BoundingBox<float> & bbox) {
    const cv::Point_<float> origin(0, 0);
    const cv::Point_<float> bboxPt2(
            bbox.points[1].x - bbox.points[0].x,
            bbox.points[1].y - bbox.points[0].y);

    return std::unique_ptr<const BoundingBox<float>>(new BoundingBox<float>(
            origin, bboxPt2));
}

std::unique_ptr<const ScanRegion<float>> getWiaBoundingBox(
        const ScanRegion<float> & bbox) {
    const cv::Point_<float> bboxPt2(
            bbox.points[1].x - bbox.points[0].x,
            bbox.points[1].y - bbox.points[0].y);

    return std::unique_ptr<const ScanRegion<float>>(new ScanRegion<float>(
            bbox.points[0], bboxPt2));
}

} /* namespace test */

} /* namespace dmscanlib */
