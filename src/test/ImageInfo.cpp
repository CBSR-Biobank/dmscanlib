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

ImageInfo::ImageInfo(const std::string & filename) : decodedWellCount(0) {
	std::ifstream file;
	file.open(filename);

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
				imageFilename = tokens[0];
			} else if (linecnt == 1) {
				if (tokens.size() != 2) {
					throw std::logic_error("two tokens expected");
				}
				double width = stringToDouble(tokens[0]);
				double height = stringToDouble(tokens[1]);
				dimensions = std::unique_ptr<const Point<double> >(
						new Point<double >(width, height));
			} else {
				if ((tokens.size() != 9) && (tokens.size() != 10)) {
					throw std::logic_error("invalid label line");
				}

				// get the rectangle points
				std::vector<std::unique_ptr<const Point<double> > > points;
				points.resize(4);
				for (unsigned i = 0; i < 4; ++i) {
					double x = stringToDouble(tokens[2 * i + 1]);
					double y = stringToDouble(tokens[2 * i + 2]);
					points[i] = std::unique_ptr<const Point<double> >(
							new Point<double >(x, y));
				}
				std::unique_ptr<const Rect<double> > rect(new Rect<double>(*points[0], *points[1],
						*points[2], *points[3]));

				std::string decodedMsg;
				if (tokens.size() == 10) {
					++decodedWellCount;
					decodedMsg = tokens[9];
				}

				wells.insert(std::make_pair(tokens[0],
						std::make_pair(std::move(rect), decodedMsg)));
			}

			++linecnt;
		}
	}
	file.close();
}

void ImageInfo::toCout() {
	std::cout << imageFilename << ": ";
	std::cout << *dimensions << std::endl;
	std::map<const std::string, std::pair<std::unique_ptr<const Rect<double> >, const std::string>>::iterator ii = wells.begin();

	for (; ii != wells.end(); ++ii) {
		const Rect<double> & rect = *ii->second.first;
		std::cout << ii->first << ":" << rect << ":" << ii->second.second << std::endl;
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


double ImageInfo::stringToDouble(const std::string& s) {
	std::istringstream i(s);
	double x;
	if (!(i >> x))
		return 0;
	return x;
}

const Rect<double> & ImageInfo::getWellRect(const std::string & label) {
	if (wells.find(label) == wells.end()) {
		throw std::invalid_argument("label not present: " + label);
	}

	return *wells[label].first;
}

const std::string * ImageInfo::getBarcodeMsg(const std::string & label) {
	if (wells.find(label) == wells.end()) {
		return NULL;
	}

	return &wells[label].second;
}

} /* namespace test */
} /* namespace dmscanlib */
