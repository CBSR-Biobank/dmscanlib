/*
 * ImageInfo.cpp
 *
 *  Created on: 2012-11-09
 *      Author: nelson
 */

#include "ImageInfo.h"

#include<iostream>
#include<fstream>
#include <sstream>
#include <algorithm>
#include <iterator>

namespace dmscanlib {
namespace test {

ImageInfo::ImageInfo(const std::string & fname) :
		filename(fname),
		decodedWellCount(0) {
	std::ifstream file;
	file.open(filename);

	fileValid = file.good();

	if (file.is_open()) {
		unsigned linecnt = 0;
		while (!file.eof()) {
			std::string line;
			file >> line;

			std::vector<std::string> tokens;
			split(line, ',', tokens);

			if (tokens.size() == 0) break;

			if (linecnt == 0) {
				if (tokens.size() != 1) {
					throw std::logic_error("single token expected");
				}
				setImageFilename(tokens[0]);
			} else if (linecnt == 1) {
				if (tokens.size() != 4) {
					throw std::logic_error("four tokens expected");
				}
				unsigned x = stringToUnsigned(tokens[0]);
				unsigned y = stringToUnsigned(tokens[1]);
				unsigned width = stringToUnsigned(tokens[2]);
				unsigned height = stringToUnsigned(tokens[3]);

				Point<unsigned> pt1(x, y);
				Point<unsigned> pt2(width, height);
				BoundingBox<unsigned> bbox(pt1, pt2);

				boundingBox = std::unique_ptr<const BoundingBox<unsigned> >(
						new BoundingBox<unsigned>(pt1, pt2));
			} else if (linecnt == 2) {
				if (tokens.size() != 2) {
					throw std::logic_error("two tokens expected");
				}
				palletRows = stringToUnsigned(tokens[0]);
				palletCols = stringToUnsigned(tokens[1]);

			} else {
				if ((tokens.size() < 1) && (tokens.size() > 2)) {
					throw std::logic_error("invalid label line");
				}

				std::string decodedMsg;
				if (tokens.size() == 2) {
					++decodedWellCount;
					decodedMsg = tokens[1];
				}

				wells.insert(std::make_pair(tokens[0], decodedMsg));
			}

			++linecnt;
		}
	}
	file.close();
}

void ImageInfo::setImageFilename(std::string & basename) {
	imageFilename = basename;
	std::ifstream file;
	file.open(imageFilename);
	imageFileValid = file.good();
	file.close();
}

void ImageInfo::toCout() {
	std::cout << imageFilename << ": ";
	std::cout << *boundingBox << std::endl;
	std::map<const std::string, const std::string>::iterator ii = wells.begin();

	for (; ii != wells.end(); ++ii) {
		std::cout << ii->first << "," << ii->second << std::endl;
	}
}

std::vector<std::string> & ImageInfo::split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while(std::getline(ss, item, delim)) {
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

} /* namespace test */
} /* namespace dmscanlib */
