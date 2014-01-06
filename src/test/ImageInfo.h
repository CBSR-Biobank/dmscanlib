/*
 * ImageInfo.h
 *
 *  Created on: 2012-11-09
 *      Author: nelson
 */

#ifndef IMAGEINFO_H_
#define IMAGEINFO_H_

#include "geometry.h"
#include "decoder/WellRectangle.h"

#include <string>
#include <map>
#include <memory>

namespace dmscanlib {

namespace test {

class ImageInfo {
public:
	ImageInfo(const std::string & filename);
	virtual ~ImageInfo() {}

	const std::string getImageFilename() const { return imageFilename; }
	const Rect<double> & getWellRect(const std::string & label);
	const std::string * getBarcodeMsg(const std::string & label);

	const BoundingBox<unsigned> & getBoundingBox() const {
		return *boundingBox;
	}

	const unsigned getDecodedWellCount() const {
		return decodedWellCount;
	}

	const unsigned getPalletRows() const {
		return palletRows;
	}

	const unsigned getPalletCols() const {
		return palletCols;
	}


private:
	std::vector<std::string> & split(const std::string &s, char delim,
			std::vector<std::string> &elems);
	unsigned stringToUnsigned(const std::string& s);
	void toCout();

	std::string imageFilename;
	std::unique_ptr<const BoundingBox<unsigned> > boundingBox;
	std::map<const std::string, const std::string> wells;
	unsigned palletRows;
	unsigned palletCols;
	unsigned decodedWellCount;
};

} /* namespace test */
} /* namespace dmscanlib */

#endif /* IMAGEINFO_H_ */
