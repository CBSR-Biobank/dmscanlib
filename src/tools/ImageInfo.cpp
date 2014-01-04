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
#include "dib/Dib.h"
#include "test/TestCommon.h"

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
DEFINE_string(palletSize, "8x12", "comma-seperated list of pallet sizes. "
		"Valid sizer are 8x12, 10x10, and 1x1");

enum PalletSize { PSIZE_8x12, PSIZE_10x10, PSIZE_1x1, PSIZE_MAX };

class ImageInfo {
public:
	ImageInfo(const std::string & _filename, const PalletSize _palletSize,
			const bool _decode);
	virtual ~ImageInfo() {}

	static PalletSize getPalletSizeFromString(std::string & palletSizeStr);

private:
	void generateWells();
	void decodeImage(std::vector<std::unique_ptr<const WellRectangle<double> > > & wells,
			std::map<std::string, std::string> & decodedMessages);

	static const std::pair<const unsigned, const unsigned> RowColsForPalletSize[PSIZE_MAX];

	const std::string & filename;
	const PalletSize palletSize;
	const bool decode;
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
		{ 1 , 1 }
};


ImageInfo::ImageInfo(const std::string & _filename, const PalletSize _palletSize,
		const bool _decode) :
		filename(_filename), palletSize(_palletSize), decode(_decode)
{
	generateWells();
}

PalletSize ImageInfo::getPalletSizeFromString(std::string & palletSizeStr) {
	PalletSize palletSize = PSIZE_MAX;

	if (palletSizeStr.compare("8x12") == 0) {
		palletSize = PSIZE_8x12;
	} else if (palletSizeStr.compare("10x10") == 0) {
		palletSize = PSIZE_10x10;
	} else if (palletSizeStr.compare("1x1") == 0) {
		palletSize = PSIZE_1x1;
	}

	return palletSize;
}

void ImageInfo::generateWells() {
    std::vector<std::unique_ptr<const WellRectangle<double> > > wells;

    test::getWellRectsForPalletImage(filename,
    		RowColsForPalletSize[palletSize].first,
    		RowColsForPalletSize[palletSize].second,
    		wells);

	std::map<std::string, std::string> decodedMessages;
	if (decode) {
		decodeImage(wells, decodedMessages);
	}

	Dib dib;
	dib.readFromFile(filename);

	std::cout << basename((char *) filename.c_str()) << std::endl;
	std::cout << dib.getWidth() << "," << dib.getHeight() << std::endl;

	for(unsigned i = 0, n = wells.size(); i < n; ++i) {
		const WellRectangle<double> & wellRect = *wells[i];

		// remove spaces and brackets from what Rect returns for stream out
		std::stringstream ss;
		ss << wellRect.getRectangle();
		std::string rectString(ss.str());
		rectString.erase(remove_if(rectString.begin(), rectString.end(), IsOutputInvalidChar()),
				rectString.end());

		std::cout << wellRect.getLabel() << "," << rectString;
		if (decodedMessages.find(wellRect.getLabel()) != decodedMessages.end()) {
			std::cout << "," << decodedMessages[wellRect.getLabel()];
		}
		std::cout << std::endl;
	}
}

void ImageInfo::decodeImage(
		std::vector<std::unique_ptr<const WellRectangle<double> > > & wellRects,
		std::map<std::string, std::string> & decodedMessages) {
	DmScanLib dmScanLib(0);

    std::unique_ptr<DecodeOptions> decodeOptions = test::getDefaultDecodeOptions();
    int result = dmScanLib.decodeImageWells(filename.c_str(), *decodeOptions, wellRects);

    if (result != SC_SUCCESS) {
    	std::cerr << "could not decode image: " << filename << std::endl;
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

	PalletSize palletSize = ImageInfo::getPalletSizeFromString(FLAGS_palletSize);

	if (palletSize == PSIZE_MAX) {
		std::cerr << "invalid pallet size specified: " << FLAGS_palletSize
				<< std::endl;
	}

	const std::string filename(argv[1]);

	ImageInfo imageInfo(filename, palletSize, FLAGS_decode);
}
