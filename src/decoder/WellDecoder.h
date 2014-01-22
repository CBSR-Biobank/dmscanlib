#ifndef __INC_PALLET_CELL_H
#define __INC_PALLET_CELL_H

#include "WellRectangle.h"

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
    WellDecoder(
            const Decoder & decoder,
            std::unique_ptr<const WellRectangle> _wellRectangle);

    virtual ~WellDecoder();

    virtual void run();

    bool isFinished();

    const std::string & getLabel() const {
        return wellRectangle->getLabel();
    }

    const std::string & getMessage() const {
        return message;
    }

    void setMessage(const char * message, int messageLength);

    const cv::Rect getWellRectangle() const;

    const std::vector<cv::Point> & getDecodedQuad() const {
        return decodedQuad;
    }

    void setDecodeQuad(const cv::Point2f (&points)[4]);

    const bool getDecodeValid() {
        return message.empty();
    }

private:
    const Decoder & decoder;
    std::unique_ptr<const WellRectangle> wellRectangle;
    std::unique_ptr<const Image> wellImage;
    cv::Rect rectangle;
    std::vector<cv::Point> decodedQuad;
    std::string message;

    friend std::ostream & operator<<(std::ostream & os, const WellDecoder & m);
};

} /* namespace */

#endif /* __INC_PALLET_CELL_H */
