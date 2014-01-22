#ifndef WELLCOORDINATES_H_
#define WELLCOORDINATES_H_

/*
 * WellCoordinates.h
 *
 *  Created on: 2012-10-12
 *      Author: loyola
 */

#include "geometry.h"

#include <ostream>
#include <string>
#include <opencv/cv.h>

namespace dmscanlib {

class WellRectangle {
public:
    WellRectangle(const char * label, unsigned x, unsigned y, unsigned width, unsigned height);

    virtual ~WellRectangle() {
    }

    const std::string & getLabel() const {
        return label;
    }

    const cv::Rect & getRectangle() const {
        return rect;
    }


private:
    const std::string label;
    const cv::Rect rect;

    friend std::ostream & operator<<(std::ostream & os, const WellRectangle & m);
};

std::ostream & operator<<(std::ostream &os, const WellRectangle & m) {
    os << m.label << " - " << m.rect;
    return os;
}

} /* namespace */

#endif /* WELLCOORDINATES_H_ */
