#ifndef DECODEOPTIONS_H_
#define DECODEOPTIONS_H_

/*
 * DecodeOptions.h
 *
 *  Created on: 2012-10-11
 *      Author: nelson
 */

class Decoder;

#include <jni.h>
#include <ostream>

using namespace std;

class DecodeOptions {
public:
	DecodeOptions(double scanGap, long squareDev, long edgeThresh,
		    long corrections, double cellDistance);
	DecodeOptions(JNIEnv *env, jobject scanRegionObj);
	virtual ~DecodeOptions();

private:

	friend class Decoder;
	friend ostream & operator<<(std::ostream & os, const DecodeOptions & m);

    double scanGap;
    long squareDev;
    long edgeThresh;
    long corrections;
    double cellDistance;
};

#endif /* DECODEOPTIONS_H_ */
