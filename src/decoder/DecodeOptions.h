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

namespace dmscanlib {

class Decoder;

class DecodeOptions {
public:
	DecodeOptions(double scanGap, long squareDev, long edgeThresh,
		    long corrections, long shrink, double cellDistance);
	DecodeOptions(JNIEnv *env, jobject scanRegionObj);
	virtual ~DecodeOptions();

private:

	friend class Decoder;
	friend std::ostream & operator<<(std::ostream & os, const DecodeOptions & m);

    double scanGap;
    long squareDev;
    long edgeThresh;
    long corrections;
    long shrink;
    double cellDistance;
};


std::ostream & operator<<(std::ostream & os, const DecodeOptions & m);

} /* namespace */

#endif /* DECODEOPTIONS_H_ */
