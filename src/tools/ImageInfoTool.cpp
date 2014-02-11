/*
 * ImageInfo.cpp
 *
 *  Created on: 2012-11-08
 *      Author: nelson
 */

#include "DmScanLib.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellRectangle.h"
#include "decoder/WellDecoder.h"
#include "test/TestCommon.h"
#include "test/ImageInfo.h"
#include "Image.h"

#include <gflags/gflags.h>
#include <string>
#include <iostream>
#include <memory>
#include <libgen.h>
#include <sstream>

namespace dmscanlib {

namespace test {

std::string usage(
		"Creates image information files for images containing DataMatrix "
		"tubes. These information files are then used for dmscanlib unit "
		"tests.\n\n"
		"Sample usage:\n"
		);

DEFINE_bool(decode, true, "include decoded barcode messages.");
DEFINE_string(orientation, "landscape", "image orientation: \"landscape\" or \"portrait\"");
DEFINE_string(position, "bottom", "where the barcodes are on a tube: \"top\", \"bottom\"");
DEFINE_string(palletSize, "8x12", "comma-seperated list of pallet sizes. "
		"Valid sizes are \"8x12\", \"10x10\", \"12x12\", \"9x9\", and \"1x1\"");

class ImageInfoWriter : public ImageInfo {
public:
    ImageInfoWriter(
            const std::string & _imageFilename,
            const cv::Rect & _boundingBox,
            Orientation _orientation,
            BarcodePosition _barcodePosition,
            PalletSize _palletSize)  :
                ImageInfo()
    {
        filename.assign("");
        imageFilename.assign(_imageFilename);
        fileValid = true;
        boundingBox = std::unique_ptr<const cv::Rect>(new cv::Rect(_boundingBox));
        orientation = _orientation;
        barcodePosition = _barcodePosition;
        palletSize = _palletSize;
        decodedWellCount = 0;

        switch (palletSize) {
        case PSIZE_8x12: palletRows = 8; palletCols = 12; break;
        case PSIZE_10x10: palletRows = 10; palletCols = 10; break;
        case PSIZE_12x12: palletRows = 12; palletCols = 12; break;
        case PSIZE_9x9: palletRows = 9; palletCols = 9; break;
        case PSIZE_1x1: palletRows = 1; palletCols = 1; break;
        default:
            throw std::logic_error("invalid pallet size");
        }

    }

    const void setBarcodeMsg(
            const std::string & label,
            const std::string & decodedMessage) {
        wells.insert(std::make_pair(label, decodedMessage));
    }

};

class ImageInfoTool {
public:
	ImageInfoTool(
	        const std::string & _filename,
	        Orientation _orientation,
	        BarcodePosition _position,
	        const PalletSize _palletSize,
			const bool _decode);
	virtual ~ImageInfoTool() {}

    static Orientation getOrientationFromString(std::string & orientationStr);
    static BarcodePosition BarcodePositionFromString(std::string & positionStr);
	static PalletSize getPalletSizeFromString(std::string & palletSizeStr);

private:
	void generateWells();
	void decodeImage(
	        std::vector<std::unique_ptr<const WellRectangle> > & wells,
			std::map<std::string, std::string> & decodedMessages);

	static const std::pair<const unsigned, const unsigned> RowColsForPalletSize[PSIZE_MAX];

	const std::string & filename;
    const Orientation orientation;
    const BarcodePosition position;
	const PalletSize palletSize;
	const bool decode;
    std::vector<std::unique_ptr<const WellRectangle> > wells;
};

class IsOutputInvalidChar {
public:
	bool operator() (const int & value) {
		return (value == ' ') || (value == '(') || (value == ')');
	}
};

const std::pair<const unsigned, const unsigned> ImageInfoTool::RowColsForPalletSize [PSIZE_MAX] = {
		{ 8, 12 },
		{ 10, 10 },
        { 12, 12 },
        { 9, 9 },
		{ 1 , 1 }
};


ImageInfoTool::ImageInfoTool(
        const std::string & _filename,
        Orientation _orientation,
        BarcodePosition _position,
        const PalletSize _palletSize,
		const bool _decode) :
		filename(_filename),
        orientation(_orientation),
        position(_position),
		palletSize(_palletSize),
		decode(_decode)
{
	generateWells();
}

void ImageInfoTool::generateWells() {
	std::map<std::string, std::string> decodedMessages;

	Image dib(filename);
	cv::Size size = dib.size();
    const cv::Rect boundingBox(0, 0, size.width, size.height);

	unsigned rows, cols;

	if (orientation == LANDSCAPE) {
	    rows = RowColsForPalletSize[palletSize].first;
	    cols = RowColsForPalletSize[palletSize].second;
	} else {
        rows = RowColsForPalletSize[palletSize].second;
        cols = RowColsForPalletSize[palletSize].first;
	}

    test::getWellRectsForBoundingBox(boundingBox, rows, cols, orientation, position, wells);

    if (decode) {
        decodeImage(wells, decodedMessages);
    }

    ImageInfoWriter imageInfo(
            filename,
            boundingBox,
            orientation,
            position,
            palletSize);

    const std::string empty;

	for(unsigned i = 0, n = wells.size(); i < n; ++i) {
		const WellRectangle & wellRect = *wells[i];
		if (decodedMessages.find(wellRect.getLabel()) != decodedMessages.end()) {
		    imageInfo.setBarcodeMsg(wellRect.getLabel(), decodedMessages[wellRect.getLabel()]);
		} else {
            imageInfo.setBarcodeMsg(wellRect.getLabel(), empty);
		}
	}
    std::cout << imageInfo;
}

void ImageInfoTool::decodeImage(
		std::vector<std::unique_ptr<const WellRectangle> > & wellRects,
		std::map<std::string, std::string> & decodedMessages) {
	DmScanLib dmScanLib(0);

    std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();
    int result = dmScanLib.decodeImageWells(filename.c_str(), *decodeOptions, wellRects);

    if (result != SC_SUCCESS) {
    	std::cerr << "could not decode image: " << filename << ", result: " << result << std::endl;
    }

	if (dmScanLib.getDecodedWellCount() > 0) {
	    const std::map<std::string, const WellDecoder *> & decodedWells = dmScanLib.getDecodedWells();
		for (std::map<std::string, const WellDecoder *>::const_iterator ii = decodedWells.begin();
				ii != decodedWells.end(); ++ii) {
			const WellDecoder & decodedWell = *(ii->second);
			decodedMessages[decodedWell.getLabel()] = decodedWell.getMessage();
		}
	}
}

} /* namespace */

} /* namespace */

using namespace dmscanlib;
using namespace test;

int main(int argc, char **argv) {
	usage.append(argv[0]).append(" <IMAGE_FILE>");

	google::SetUsageMessage(usage);
	google::ParseCommandLineFlags(&argc, &argv, true);

	if (argc != 2) {
		std::cout << "image file name not specified." << std::endl;
		return 1;
	}

	Orientation orientation = DmScanLib::getOrientationFromString(FLAGS_orientation);
	BarcodePosition position = DmScanLib::getBarcodePositionFromString(FLAGS_position);
	PalletSize palletSize = DmScanLib::getPalletSizeFromString(FLAGS_palletSize);

	const std::string filename(argv[1]);

	ImageInfoTool imageInfo(filename, orientation, position, palletSize, FLAGS_decode);
}
