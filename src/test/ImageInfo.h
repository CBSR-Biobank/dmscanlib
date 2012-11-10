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
	void getWellRects(std::vector<std::unique_ptr<const WellRectangle<double> > > & wellRects) const;
	const Rect<double> & getWellRect(const std::string & label);
	const std::string * getBarcodeMsg(const std::string & label);

	const unsigned getTotalWells() const {
		return wells.size();
	}


private:
	std::vector<std::string> & split(const std::string &s, char delim,
			std::vector<std::string> &elems);
	double stringToDouble(const std::string& s);
	void toCout();

	std::string imageFilename;
	std::unique_ptr<const Point<double> > dimensions;
	std::map<const std::string, std::pair<std::unique_ptr<const Rect<double> >, const std::string>> wells;
};

} /* namespace test */
} /* namespace dmscanlib */

#endif /* IMAGEINFO_H_ */
