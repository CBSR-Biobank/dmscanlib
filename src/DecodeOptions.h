#ifndef DECODEOPTIONS_H_
#define DECODEOPTIONS_H_

/*
 * DecodeOptions.h
 *
 *  Created on: 2012-10-11
 *      Author: nelson
 */

#include <jni.h>

class DecodeOptions {
public:
	DecodeOptions(double scanGap, long squareDev, long edgeThresh,
		    long corrections, double cellDistance);
	DecodeOptions(JNIEnv *env, jobject scanRegionObj);
	virtual ~DecodeOptions();

    double scanGap;
    long squareDev;
    long edgeThresh;
    long corrections;
    double cellDistance;
};

#endif /* DECODEOPTIONS_H_ */
