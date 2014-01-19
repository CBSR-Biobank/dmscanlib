#ifndef DECODEOPTIONS_H_
#define DECODEOPTIONS_H_

/*
 * DecodeOptions.h
 *
 *  Created on: 2012-10-11
 *      Author: nelson
 */

#include <jni.h>
#include <ostream>
#include <memory>

namespace dmscanlib {

class Decoder;

class DecodeOptions {
public:
    DecodeOptions(
            const double minEdgeFactor,
            const double maxEdgeFactor,
            const double scanGapFactor,
            const long squareDev,
            const long edgeThresh,
            const long corrections,
            const long shrink);
    virtual ~DecodeOptions();

    static std::unique_ptr<DecodeOptions> getDecodeOptionsViaJni(
            JNIEnv *env,
            jobject decodeOptionsObj);

    const double minEdgeFactor;
    const double maxEdgeFactor;
    const double scanGapFactor;
    const long squareDev;
    const long edgeThresh;
    const long corrections;
    const long shrink;

private:
    friend class Decoder;
    friend std::ostream & operator<<(std::ostream & os, const DecodeOptions & m);
};

std::ostream & operator<<(std::ostream & os, const DecodeOptions & m);

} /* namespace */

#endif /* DECODEOPTIONS_H_ */
