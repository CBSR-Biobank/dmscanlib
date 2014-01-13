/*
 * TestCommon.cpp
 *
 *  Created on: 2012-11-05
 *      Author: nelson
 */

#include "test/TestCommon.h"
#include "DmScanLib.h"
#include "decoder/WellRectangle.h"
#include "decoder/DecodeOptions.h"
#include "dib/Dib.h"

#include <sstream>

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
	if (dp == NULL) return false;

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
				getTestImageFileNames(subdirname, filenames);
			} else{
				std::wstring basenamew((wchar_t *)fdFile.cFileName);
				std::string basename;
				basename.assign(basenamew.begin(), basenamew.end());

				if (basename.find(".bmp") != std::string::npos) {
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
		std::vector<std::unique_ptr<const WellRectangle<double> > > & wellRects) {

	double wellWidth = bbox.getWidth() / static_cast<double>(cols);
	double wellHeight = bbox.getHeight() / static_cast<double>(rows);

	Point<double> startPoint(
			static_cast<double>(bbox.points[0].x),
			static_cast<double>(bbox.points[0].y));
    Point<double> horTranslation(static_cast<double>(wellWidth), 0);
    Point<double> verTranslation(0, static_cast<double>(wellHeight));

    // round off the bounding box so image dimensions are not exceeded
	Point<double> pt2(0.999 * wellWidth, 0.999 * wellHeight);
	BoundingBox<double> startingWellBbox(
			startPoint,
			*pt2.translate(startPoint));

    for (unsigned row = 0; row < rows; ++row) {
	std::unique_ptr<const Point<double>> scaledVertTranslation = verTranslation.scale(row);
        std::unique_ptr<const BoundingBox<double> > bboxTranslated =
			startingWellBbox.translate(*scaledVertTranslation);

        for (unsigned col = 0; col < cols; ++col) {
        	std::ostringstream label;
        	label << (char) ('A' + row) << cols - col;

            std::unique_ptr<WellRectangle<double> > wellRect(
			new WellRectangle<double>(label.str().c_str(), *bboxTranslated));
            VLOG(3) << *wellRect;
            wellRects.push_back(std::move(wellRect));
            bboxTranslated = bboxTranslated->translate(horTranslation);
        }
    }
}

std::unique_ptr<DecodeOptions> getDefaultDecodeOptions() {
    const long squareDev = 15;
    const long edgeThresh = 5;
    const long corrections = 10;
    const long shrink = 1;

	return std::unique_ptr<DecodeOptions>(new DecodeOptions (squareDev, edgeThresh, corrections, shrink));
}

int decodeImage(std::string fname, DmScanLib & dmScanLib, unsigned rows, unsigned cols) {
    std::vector<std::unique_ptr<const WellRectangle<double> > > wellRects;

	Dib image;
	bool readResult = image.readFromFile(fname);
	if (!readResult) {
		throw std::invalid_argument("could not load image");
	}

	Point<unsigned> pt1(0, 0);
	Point<unsigned> pt2(image.getWidth(), image.getHeight());
	BoundingBox<unsigned> bbox(pt1, pt2);

	getWellRectsForBoundingBox(bbox, rows, cols, wellRects);

    std::unique_ptr<DecodeOptions> decodeOptions = getDefaultDecodeOptions();
    return dmScanLib.decodeImageWells(fname.c_str(), *decodeOptions, wellRects);
}

// bbox here has to start at (0,0)
std::unique_ptr<const BoundingBox<double>> getWellsBoundingBox(
	const BoundingBox<double> & bbox) {
	const Point<double> origin(0, 0);
	std::unique_ptr<const Point<double> > bboxPt1Neg(bbox.points[0].scale(-1));

	return std::unique_ptr<const BoundingBox<double>>(new BoundingBox<double>(
		origin, *bbox.points[1].translate(*bboxPt1Neg)));
}

std::unique_ptr<const ScanRegion<double>> getWiaBoundingBox(
	const ScanRegion<double> & bbox) {
	std::unique_ptr<const Point<double> > bboxPt1Neg(bbox.points[0].scale(-1));

	return std::unique_ptr<const ScanRegion<double>>(new ScanRegion<double>(
		bbox.points[0], *bbox.points[1].translate(*bboxPt1Neg)));
}


} /* namespace */

} /* namespace */
