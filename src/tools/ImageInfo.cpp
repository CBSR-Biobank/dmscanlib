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
#include "Image.h"

#include <gflags/gflags.h>
#include <string>
#include <iostream>
#include <memory>
#include <libgen.h>
#include <sstream>

namespace dmscanlib {

namespace imageinfo {

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

class ImageInfo {
public:
	ImageInfo(
	        const std::string & _filename,
	        Orientation _orientation,
	        BarcodePosition _position,
	        const PalletSize _palletSize,
			const bool _decode);
	virtual ~ImageInfo() {}

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

const std::pair<const unsigned, const unsigned> ImageInfo::RowColsForPalletSize [PSIZE_MAX] = {
		{ 8, 12 },
		{ 10, 10 },
        { 12, 12 },
        { 9, 9 },
		{ 1 , 1 }
};


ImageInfo::ImageInfo(
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

void ImageInfo::generateWells() {
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

    ImageInfo imageInfo(
            filename,
            boundingBox,
            orientation,
            position,
            palletSize);

	for(unsigned i = 0, n = wells.size(); i < n; ++i) {
		const WellRectangle & wellRect = *wells[i];

		std::cout << wellRect.getLabel();
		if (decodedMessages.find(wellRect.getLabel()) != decodedMessages.end()) {
			std::cout << "," << decodedMessages[wellRect.getLabel()];
		}
		std::cout << std::endl;
	}
}

void ImageInfo::decodeImage(
		std::vector<std::unique_ptr<const WellRectangle> > & wellRects,
		std::map<std::string, std::string> & decodedMessages) {
	DmScanLib dmScanLib(1);

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
using namespace imageinfo;

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
	PalletSize palletSize = ImageInfo::getPalletSizeFromString(FLAGS_palletSize);

	const std::string filename(argv[1]);

	ImageInfo imageInfo(filename, orientation, position, palletSize, FLAGS_decode);
}
