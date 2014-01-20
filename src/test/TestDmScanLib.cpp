/*
 * TestDmScanLib.cpp
 *
 *  Created on: 2012-11-04
 *      Author: nelson
 */

#include "DmScanLib.h"
#include "Image.h"
#include "decoder/Decoder.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellDecoder.h"
#include "test/TestCommon.h"
#include "test/ImageInfo.h"
#include "decoder/DmtxDecodeHelper.h"

#include <dmtx.h>

#include <algorithm>
#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <stdexcept>
#include <stddef.h>
#include <sys/types.h>
#include <sstream>
#include <iostream>
#include <fstream>

#include <glog/logging.h>
#include <gtest/gtest.h>

using namespace dmscanlib;

namespace {

TEST(TestDmScanLib, invalidRects) {
	FLAGS_v = 0;

    std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();
    std::vector<std::unique_ptr<const WellRectangle<double> > > wellRects;
	DmScanLib dmScanLib(1);
    int result = dmScanLib.decodeImageWells("testImages/8x12/96tubes.bmp", *decodeOptions,
            wellRects);
	EXPECT_EQ(SC_INVALID_NOTHING_DECODED, result);
}

TEST(TestDmScanLib, invalidImage) {
	FLAGS_v = 0;

	Point<double> pt1(0,0);
	Point<double> pt2(10,10);
	BoundingBox<double> bbox(pt1, pt2);
	std::unique_ptr<const WellRectangle<double> > wrect(new WellRectangle<double>("label", bbox));

	std::vector<std::unique_ptr<const WellRectangle<double> > > wellRects;
    wellRects.push_back(std::move(wrect));

    std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();
	DmScanLib dmScanLib(1);
	int result = dmScanLib.decodeImageWells("xyz.bmp", *decodeOptions, wellRects);
	EXPECT_EQ(SC_INVALID_IMAGE, result);
}

TEST(TestDmScanLib, decodeImage) {
    FLAGS_v = 2;

    //std::string fname("testImages/8x12/hardscan.bmp");
    //std::string fname("testImages/8x12/plate.bmp");
    //std::string fname("/home/nelson/Desktop/ohs_crash_image001.bmp");
    //std::string fname("testImages/10x10/10x10.bmp");
    std::string fname("testImages/10x10/10x10.bmp");

	DmScanLib dmScanLib(1);
    int result = test::decodeImage(fname, dmScanLib, 10, 10);

	EXPECT_EQ(SC_SUCCESS, result);
	EXPECT_TRUE(dmScanLib.getDecodedWellCount() > 0);

	if (dmScanLib.getDecodedWellCount() > 0) {
		VLOG(1) << "number of wells decoded: " << dmScanLib.getDecodedWells().size();
	}
}

void writeAllDecodeResults(std::vector<std::string> & testResults, bool append = false) {
    std::ofstream ofile;
    if (append) {
        ofile.open("all_images_results.csv", std::ios::app);
    } else {
        ofile.open("all_images_results.csv");
    }

	for (unsigned i = 0, n = testResults.size(); i < n; ++i) {
		ofile <<  testResults[i] << std::endl;
	}
	ofile.close();
}

// check that the decoded message matches the one in the "nfo" file
void checkDecodeInfo(DmScanLib & dmScanLib, dmscanlib::test::ImageInfo & imageInfo) {
    const std::map<std::string, const WellDecoder *> & decodedWells = dmScanLib.getDecodedWells();
    for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells.begin();
            ii != decodedWells.end(); ++ii) {
        const WellDecoder & decodedWell = *(ii->second);
        const std::string & label = decodedWell.getLabel();
        const std::string * nfoDecodedMsg = imageInfo.getBarcodeMsg(label);

        if ((decodedWell.getMessage().length() > 0) && (nfoDecodedMsg != NULL)) {
            EXPECT_EQ(*nfoDecodedMsg, decodedWell.getMessage()) << "label: " << label;
        }
    }
}

class DecodeTestResult {
public:
    bool infoFileValid;
	int decodeResult;
    unsigned totalTubes;
    unsigned totalDecoded;
    double decodeTime;

    DecodeTestResult() {
    }

};

std::unique_ptr<DecodeTestResult> decodeFromInfo(
        std::string & infoFilename,
        DecodeOptions & decodeOptions,
        DmScanLib & dmScanLib) {

    std::unique_ptr<DecodeTestResult> testResult(new DecodeTestResult());

    dmscanlib::test::ImageInfo imageInfo(infoFilename);
    testResult->infoFileValid = imageInfo.isValid();

    if (testResult->infoFileValid) {
        testResult->totalTubes = imageInfo.getDecodedWellCount();
        std::vector<std::unique_ptr<const WellRectangle<double> > > wellRects;
        test::getWellRectsForBoundingBox(
                imageInfo.getBoundingBox(),
                imageInfo.getPalletRows(),
                imageInfo.getPalletCols(),
                wellRects);

        util::DmTime start;
        testResult->decodeResult = dmScanLib.decodeImageWells(
                imageInfo.getImageFilename().c_str(),
                decodeOptions,
                wellRects);
        util::DmTime end;

        testResult->decodeTime = end.difftime(start)->getTime();

        if (testResult->decodeResult == SC_SUCCESS) {
            testResult->totalDecoded = dmScanLib.getDecodedWellCount();
            checkDecodeInfo(dmScanLib, imageInfo);
        }
    }

    return testResult;

}

TEST(TestDmScanLib, decodeFromInfo) {
    FLAGS_v = 3;

    std::string infoFilename("testImageInfo/8x12/hardscan.nfo");

    std::unique_ptr<DecodeOptions> defaultDecodeOptions = test::getDefaultDecodeOptions();
    DecodeOptions decodeOptions(
            0.2,
            0.3,
            0.2,
            defaultDecodeOptions->squareDev,
            defaultDecodeOptions->edgeThresh,
            defaultDecodeOptions->corrections,
            defaultDecodeOptions->shrink);

    DmScanLib dmScanLib(0);
    std::unique_ptr<DecodeTestResult> testResult =
            decodeFromInfo(infoFilename, decodeOptions, dmScanLib);

    EXPECT_TRUE(testResult->infoFileValid);
    EXPECT_EQ(SC_SUCCESS, testResult->decodeResult);
    if (testResult->decodeResult == SC_SUCCESS) {
        EXPECT_TRUE(testResult->totalDecoded > 0);
    }
}

void getDecodeInfo(DmtxDecode *dec, DmtxRegion *reg, DmtxMessage *msg) {
    CHECK_NOTNULL(dec);
    CHECK_NOTNULL(reg);
    CHECK_NOTNULL(msg);

    DmtxVector2 p00, p10, p11, p01;

    std::string message;
    message.assign((char *) msg->output, msg->outputIdx);

    VLOG(1) << "message: " << message;

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

    Point<double> pt1(p00.X, p00.Y);
    Point<double> pt2(p10.X, p10.Y);
    Point<double> pt3(p11.X, p11.Y);
    Point<double> pt4(p01.X, p01.Y);
    Rect<double> decodeRect(pt1, pt2, pt3, pt4);

    VLOG(1) << "getDecodeInfo: rect: " << decodeRect;
}

void decodeWellRect(DmtxDecode *dec) {
    DmtxRegion * reg;
    while (1) {
        reg = dmtxRegionFindNext(dec, NULL);
        if (reg == NULL) {
            break;
        }

        DmtxMessage *msg = dmtxDecodeMatrixRegion(dec, reg, 10);

        if (msg != NULL) {
            getDecodeInfo(dec, reg, msg);

            Decoder::showStats(dec, reg, msg);
            dmtxMessageDestroy(&msg);
        }
        dmtxRegionDestroy(&reg);
    }

    std::string label("LABEL");
    Decoder::writeDiagnosticImage(dec, label);
}

std::unique_ptr<decoder::DmtxDecodeHelper> createDmtxDecode(
        DmtxImage * dmtxImage,
        const BoundingBox<double> & bbox,
        int scale) {
    std::unique_ptr<decoder::DmtxDecodeHelper> dec(
            new decoder::DmtxDecodeHelper(dmtxImage, scale));

    unsigned mindim = std::min(bbox.getWidth(), bbox.getHeight());

    dec->setProperty(DmtxPropEdgeMin, static_cast<int>(0.1 * mindim));
    dec->setProperty(DmtxPropEdgeMax, static_cast<int>(0.3 * mindim));
    dec->setProperty(DmtxPropScanGap, static_cast<int>(0.15 * mindim));

    dec->setProperty(DmtxPropSymbolSize, DmtxSymbolSquareAuto);
    dec->setProperty(DmtxPropSquareDevn, 15);
    dec->setProperty(DmtxPropEdgeThresh, 5);

    return dec;
}

TEST(TestDmScanLib, opencv) {
    FLAGS_v = 1;


    cv::Mat blankKernel(3,3,CV_64F);

    blankKernel.at<double>(0,0) = 0.06185567;
    blankKernel.at<double>(0,1) = 0.12371134;
    blankKernel.at<double>(0,2) = 0.06185567;
    blankKernel.at<double>(1,0) = 0.12371134;
    blankKernel.at<double>(1,1) = 0.25773195;
    blankKernel.at<double>(1,2) = 0.12371134;
    blankKernel.at<double>(2,0) = 0.06185567;
    blankKernel.at<double>(2,1) = 0.12371134;
    blankKernel.at<double>(2,2) = 0.06185567;

    cv::Mat blurKernel(3,3,CV_64F);

    blurKernel.at<double>(0,0) = 0.0;
    blurKernel.at<double>(0,1) = 0.2;
    blurKernel.at<double>(0,2) = 0.0;
    blurKernel.at<double>(1,0) = 0.2;
    blurKernel.at<double>(1,1) = 0.2;
    blurKernel.at<double>(1,2) = 0.2;
    blurKernel.at<double>(2,0) = 0.0;
    blurKernel.at<double>(2,1) = 0.2;
    blurKernel.at<double>(2,2) = 0.0;

    //std::string fname("testImages/8x12/hardscan.bmp");
    std::string fname("/home/loyola/Desktop/single_tube.jpg");
    cv::Mat src = cv::imread(fname.c_str(), CV_LOAD_IMAGE_GRAYSCALE);

    VLOG(1) << "opencv: width: " << src.cols << ", height: " << src.rows
            << ", depth: " << src.elemSize() << ", " << src.step1();

    cv::Mat filter1Image, filter2Image;

    cv::Point anchor(-1, -1);
    double delta = 0;
    int ddepth = -1;

    cv::filter2D(src, filter1Image, ddepth , blankKernel, anchor, delta);
    cv::filter2D(filter1Image, filter2Image, ddepth , blurKernel, anchor, delta);

    std::vector<std::unique_ptr<const WellRectangle<double> > > wellRects;
    Point<unsigned> pt1(0, 0);
    Point<unsigned> pt2(filter2Image.cols, filter2Image.rows);
    BoundingBox<unsigned> bbox(pt1, pt2);

    test::getWellRectsForBoundingBox(bbox, 8, 12, wellRects);

    std::unique_ptr<const BoundingBox<double>> wellBbox =
            wellRects[2]->getRectangle().getBoundingBox();

    VLOG(1) << "opencv: well: " << *wellBbox;

    cv::Rect roi(
            static_cast<int>(wellBbox->points[0].x),
            static_cast<int>(wellBbox->points[0].y),
            static_cast<int>(wellBbox->points[1].x - wellBbox->points[0].x),
            static_cast<int>(wellBbox->points[1].y - wellBbox->points[0].y));

    cv::Mat firstWell = filter2Image(roi);

    bool r = cv::imwrite("out.bmp", firstWell);
    EXPECT_EQ(true, r);

    DmtxImage * dmtxImage = getDmtxImage(firstWell);
    std::unique_ptr<decoder::DmtxDecodeHelper> dec =
            createDmtxDecode(dmtxImage, *wellBbox, 1);
    decodeWellRect(dec->getDecode());
    dmtxImageDestroy(&dmtxImage);
}

TEST(TestDmScanLib, opencv_jpg) {
    FLAGS_v = 1;

    std::string fname("/home/loyola/Desktop/single_tube.jpg");
    Image src(fname.c_str());

    src.write("out.bmp");
    cv::Size size = src.getSize();

    Point<double> pt1(0, 0);
    Point<double> pt2(size.width, size.height);
    BoundingBox<double> bbox(pt1, pt2);

    VLOG(1) << "opencv: well: " << bbox;

    DmtxImage * dmtxImage = getDmtxImage(src.getEnhancedImage());
    std::unique_ptr<decoder::DmtxDecodeHelper> dec = createDmtxDecode(dmtxImage, bbox, 1);
    decodeWellRect(dec->getDecode());
    dmtxImageDestroy(&dmtxImage);
}

//TEST(TestDmScanLib, DISABLED_decodeAllImages) {
TEST(TestDmScanLib, decodeAllImages) {
    FLAGS_v = 1;

    std::string dirname("testImageInfo");
    std::vector<std::string> filenames;
    bool result = test::getTestImageInfoFilenames(dirname, filenames);
    EXPECT_EQ(true, result);

    std::vector<std::string> testResults;
    testResults.push_back("#filename,decoded,total,ratio,time (sec)");

    unsigned totalTubes = 0;
    unsigned totalDecoded = 0;
    double totalTime = 0;
	    std::stringstream ss;
    std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();

    for (unsigned i = 0, n = filenames.size(); i < n; ++i) {
        ss.str("");
        VLOG(1) << "test image info: " << filenames[i];

        DmScanLib dmScanLib(0);
        std::unique_ptr<DecodeTestResult> testResult =
                decodeFromInfo(filenames[i], *decodeOptions, dmScanLib);
        EXPECT_TRUE(testResult->infoFileValid);
        EXPECT_EQ(SC_SUCCESS, testResult->decodeResult);

        ss << filenames[i];

        if (testResult->decodeResult == SC_SUCCESS) {
            EXPECT_TRUE(testResult->totalDecoded > 0);

            ss << "," << testResult->totalDecoded << ","
                    << testResult->totalTubes << ","
                    << testResult->totalDecoded / static_cast<double>(testResult->totalTubes) << ","
                    << testResult->decodeTime;

            VLOG(1) << "decoded: " << testResult->totalDecoded
                              << ", total: " << testResult->totalTubes
                              << ", time taken: " << testResult->decodeTime;

            totalDecoded += testResult->totalDecoded;
            totalTubes += testResult->totalTubes;
            totalTime += testResult->decodeTime;
        }
		testResults.push_back(ss.str());
	}

    ss.str("");
    ss << "decoded:," << totalDecoded << ", total:," << totalTubes;
    testResults.push_back(ss.str());
    ss.str("");

    double avgDecodeTime = totalTime / filenames.size();
    ss << "average decode time:," << avgDecodeTime;
    testResults.push_back(ss.str());

    VLOG(1) << ", total tubes: " << totalTubes
                      << "total decoded: " << totalDecoded
                      << ", average decode time: " << avgDecodeTime;

    writeAllDecodeResults(testResults);
}

TEST(TestDmScanLib, decodeAllImagesAllParameters) {
    FLAGS_v = 1;

    std::string dirname("testImageInfo");
    std::vector<std::string> filenames;
    bool result = test::getTestImageInfoFilenames(dirname, filenames);
    EXPECT_EQ(true, result);

    std::unique_ptr<DecodeOptions> defaultDecodeOptions = test::getDefaultDecodeOptions();
    std::vector<std::string> testResults;
    testResults.push_back("#filename,decoded,total,ratio,time (sec),minEdgeFactor,maxEdgeFactor,scanGapFactor");
    writeAllDecodeResults(testResults, false); // clear previous contents of the file

    std::stringstream ss;

    for (double maxEdge = 0.15; maxEdge <= 1.05; maxEdge += 0.05) {
       for (double minEdge = 0, n = maxEdge + 0.05; minEdge < n; minEdge += 0.05) {
    		for (double scanGap = 0; scanGap <= 1.05; scanGap += 0.05) {
    			unsigned totalTubes = 0;
    			unsigned totalDecoded = 0;
    			double totalTime = 0;

    			testResults.clear();

    			DecodeOptions decodeOptions(
    					minEdge,
    					maxEdge,
    					scanGap,
                        defaultDecodeOptions->squareDev,
                        defaultDecodeOptions->edgeThresh,
                        defaultDecodeOptions->corrections,
                        defaultDecodeOptions->shrink);

                for (unsigned i = 0, n = filenames.size(); i < n; ++i) {
                    ss.str("");
                    VLOG(1) << "test image info: " << filenames[i];

                    DmScanLib dmScanLib(0);
                    std::unique_ptr<DecodeTestResult> testResult =
                            decodeFromInfo(filenames[i], decodeOptions, dmScanLib);
                    EXPECT_TRUE(testResult->infoFileValid);
                    EXPECT_EQ(SC_SUCCESS, testResult->decodeResult);

                    ss << filenames[i];

                    if (testResult->decodeResult == SC_SUCCESS) {
                        EXPECT_TRUE(testResult->totalDecoded > 0);

                        ss << "," << testResult->totalDecoded
                                << "," << testResult->totalTubes
                                << "," << testResult->totalDecoded / static_cast<double>(testResult->totalTubes)
                                << "," << testResult->decodeTime
                                << "," << minEdge
                                << "," << maxEdge
                                << "," << scanGap;

                                VLOG(1) << "decoded: " << testResult->totalDecoded
                                << ", total: " << testResult->totalTubes
                                << ", time taken: " << testResult->decodeTime
                                << ", minEdgeFactor: " << minEdge
                                << ", maxedgeFactor: " << maxEdge
                                << ", scanGapFactor: " << scanGap;

                        totalDecoded += testResult->totalDecoded;
                        totalTubes += testResult->totalTubes;
                        totalTime += testResult->decodeTime;
                    }

                    testResults.push_back(ss.str());
                }

                double avgDecodeTime = totalTime / filenames.size();
                ss.str("");
                ss << "RESULTS:,decoded:," << totalDecoded
                        << ", total:," << totalTubes
                        << ", average decode time:," << avgDecodeTime
                        << ", minEdgeFactor:," << minEdge
                        << ", maxedgeFactor:," << maxEdge
                        << ", scanGapFactor:," << scanGap;
                testResults.push_back(ss.str());
                writeAllDecodeResults(testResults, true);
            }
        }
    }
}

} /* namespace */
