#ifndef __INC_DECODE_RESULT_H
#define __INC_DECODE_RESULT_H

#include "structs.h"
#include <string>

struct DecodeResult {
    std::string msg;
    Point corners[4];
};


#endif /* __INC_DECODE_RESULT_H */
