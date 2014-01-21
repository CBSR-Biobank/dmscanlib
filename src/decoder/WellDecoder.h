#ifndef __INC_PALLET_CELL_H
#define __INC_PALLET_CELL_H

#include "WellRectangle.h"
#include "geometry.h"

#include <dmtx.h>
#include <opencv/cv.h>
#include <string>
#include <ostream>
#include <memory>
#include <OpenThreads/Mutex>
#include <OpenThreads/ScopedLock>
#include <OpenThreads/Thread>

namespace dmscanlib {

class Decoder;
class Image;
class RgbQuad;
class PalletGrid;

class WellDecoder: public ::OpenThreads::Thread {
public:
    WellDecoder(const Decoder & decoder,
            std::unique_ptr<const WellRectangle<unsigned> > _wellRectangle);

    virtual ~WellDecoder();

    virtual void run();

    bool isFinished();

    void decodeCallback(std::string & decodedMsg, cv::Point_<unsigned> (&corners)[4]);

    const std::string & getLabel() const {
        return wellRectangle->getLabel();
    }

    const std::string & getMessage() const {
        return message;
    }

    void setMessage(const char * message, int messageLength);

    const cv::Rect getWellRectangle() const;

    const cv::Rect getDecodedRectangle() const;

    void setDecodeRectangle(const Rect<float> & rect, int scale);

    const bool getDecodeValid() {
        return message.empty();
    }

private:
    const Decoder & decoder;
    std::unique_ptr<const WellRectangle<unsigned> > wellRectangle;
    std::unique_ptr<const Image> wellImage;
    std::unique_ptr<const BoundingBox<unsigned> > boundingBox;
    std::unique_ptr<const Rect<unsigned> > decodedRect;
    std::string message;

    friend std::ostream & operator<<(std::ostream & os, const WellDecoder & m);
};

} /* namespace */

#endif /* __INC_PALLET_CELL_H */
