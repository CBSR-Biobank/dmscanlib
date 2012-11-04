/*
 * TestDmScanLib.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#include "DmScanLib.h"
#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellRectangle.h"
#include "dib/Dib.h"

#include <stdexcept>
#include <stddef.h>
#include <dirent.h>
#include <sys/types.h>

#ifdef _VISUALC_
#   pragma warning(disable : 4996)
#endif

#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace dmscanlib;

namespace {

/*
 * Gets file names for all the test images in the "testImages" folder. Only MS
 * Windows bitmap files are included.
 *
 * These test images can be downloaded from
 * http://aicml-med.cs.ualberta.ca/CBSR/scanlib/testImages.tar.bz2
 */
bool getTestImageFileNames(std::string dir, std::vector<std::string> & filenames) {
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
			getTestImageFileNames(subdirname, filenames);
		} else if (dirp->d_type == DT_REG) {
			std::string basename(dirp->d_name);

			if (basename.find(".bmp") != std::string::npos) {
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

/*
 * Assumes image has 96 well plates in 8 rows by 12 columns
 */
void getWellRectsForSbsPalletImage(std::string & fname,
		std::vector<std::unique_ptr<WellRectangle<double> > > & wellRects) {

	Dib image;
	bool readResult = image.readFromFile(fname);
	if (!readResult) {
		throw std::invalid_argument("could not load image");
	}

	const double doubleDpi = static_cast<double>(image.getDpi());
	const double dotWidth = 1 / doubleDpi;
	double width = static_cast<double>(image.getWidth()) / doubleDpi;
	double height = static_cast<double>(image.getHeight()) / doubleDpi;
    double wellWidth = width / 12.0;
    double wellHeight = height / 8.0;

    Point<double> horTranslation(static_cast<double>(wellWidth), 0);
    Point<double> verTranslation(0, static_cast<double>(wellHeight));

    // round off the bounding box so image dimensions are not exceeded
	Point<double> pt1(0, 0);
	Point<double> pt2(wellWidth - dotWidth, wellHeight - dotWidth);
	BoundingBox<double> bbox(pt1, pt2);

    for (int row = 0; row < 8; ++row) {
	std::unique_ptr<const Point<double>> scaledVertTranslation = verTranslation.scale(row);
        std::unique_ptr<const BoundingBox<double> > bboxTranslated =
			bbox.translate(*scaledVertTranslation);

        for (int col = 0; col < 12; ++col) {
		std::ostringstream label;
		label << (char) ('A' + row) << 12 - col;

            std::unique_ptr<WellRectangle<double> > wellRect(
			new WellRectangle<double>(label.str().c_str(), *bboxTranslated));
            VLOG(9) << *wellRect;
            wellRects.push_back(std::move(wellRect));
            bboxTranslated = bboxTranslated->translate(horTranslation);
        }
    }
}

int decodeImage(std::string fname, DmScanLib & dmScanLib) {
    std::vector<std::unique_ptr<WellRectangle<double> > > wellRects;

    getWellRectsForSbsPalletImage(fname, wellRects);

    double scanGap = 5;
    long squareDev = 15;
    long edgeThresh = 5;
    long corrections = 10;
    long shrink = 1;

    DecodeOptions decodeOptions(scanGap, squareDev, edgeThresh, corrections,
	    shrink);
    return dmScanLib.decodeImageWells(fname.c_str(), decodeOptions, wellRects);
}

TEST(TestApp, DecodeImage) {
	FLAGS_v = 3;

    std::string fname("testImages/96tubes.bmp");

    DmScanLib dmScanLib(1);
    int result = decodeImage(fname, dmScanLib);

    EXPECT_EQ(SC_SUCCESS, result);
    EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

    if (dmScanLib.getDecodedWellCount() > 0) {
	VLOG(1) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
    }
}

TEST(TestApp, DecodeAllImages) {
	FLAGS_v = 1;

    std::string dirname("testImages");
    std::vector<std::string> filenames;
	bool result = getTestImageFileNames(dirname, filenames);
    EXPECT_EQ(true, result);

    int decodeResult;

    for (unsigned i = 0, n = filenames.size(); i < n; ++i) {
	VLOG(1) << "test image: " << filenames[i];

		util::DmTime start;
        DmScanLib dmScanLib(1);
	decodeResult = decodeImage(filenames[i], dmScanLib);
		util::DmTime end;

		std::unique_ptr<util::DmTime> difftime = end.difftime(start);

        EXPECT_EQ(SC_SUCCESS, decodeResult);
        EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

        VLOG(1) << "test image: " << filenames[i] << ", wells decoded: "
			<< dmScanLib.getDecodedWellCount()
			<< " time taken: " << *difftime << " sec";
    }
}

} /* namespace */
