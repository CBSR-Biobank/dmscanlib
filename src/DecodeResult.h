#ifndef __INC_DECODE_RESULT_H
#define __INC_DECODE_RESULT_H

#include "cv.h"

#include <string>

struct DecodeResult {
    std::string msg;
    CvPoint corners[4];
};


#endif /* __INC_DECODE_RESULT_H */
