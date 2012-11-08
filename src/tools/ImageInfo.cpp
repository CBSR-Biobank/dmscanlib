/*
 * ImageInfo.cpp
 *
 *  Created on: 2012-11-08
 *      Author: nelson
 */

#include <gflags/gflags.h>
#include <string>
#include <iostream>

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

enum PalletSize { SIZE_UNKNOWN, SIZE_8x12, SIZE_10x10, SIZE_1x1 };

class ImageInfo {
public:
	ImageInfo(const std::string & _filename, const PalletSize _palletSize,
			const bool _decode);
	virtual ~ImageInfo() {}

	static PalletSize getPalletSizeFromString(std::string & palletSizeStr);

private:

	void decodeImage();
	void generateWells();

	const std::string & filename;
	const PalletSize palletSize;
	const bool decode;
};


ImageInfo::ImageInfo(const std::string & _filename, const PalletSize _palletSize,
		const bool _decode) :
		filename(_filename), palletSize(_palletSize), decode(_decode)
{
	std::cout << "filename: " << filename << std::endl;
	std::cout << "decode: " << decode << std::endl;
	std::cout << "palletSize: " << palletSize << std::endl;
}

PalletSize ImageInfo::getPalletSizeFromString(std::string & palletSizeStr) {
	PalletSize palletSize = SIZE_UNKNOWN;

	if (palletSizeStr.compare("8x12") == 0) {
		palletSize = SIZE_8x12;
	} else if (palletSizeStr.compare("10x10") == 0) {
		palletSize = SIZE_10x10;
	} else if (palletSizeStr.compare("1x1") == 0) {
		palletSize = SIZE_1x1;
	}

	return palletSize;
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

	if (palletSize == SIZE_UNKNOWN) {
		std::cerr << "invalid pallet size specified: " << FLAGS_palletSize
				<< std::endl;
	}

	const std::string filename(argv[0]);

	ImageInfo imageInfo(filename, palletSize, FLAGS_decode);
}
