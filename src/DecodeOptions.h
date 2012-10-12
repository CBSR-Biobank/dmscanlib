/*
 * DecodeOptions.h
 *
 *  Created on: 2012-10-11
 *      Author: nelson
 */

#ifndef DECODEOPTIONS_H_
#define DECODEOPTIONS_H_

class DecodeOptions {
public:
	DecodeOptions(JNIEnv *env, jobject scanRegionObj);
	virtual ~DecodeOptions() { };

    double scanGap;
    long squareDev;
    long edgeThresh;
    long corrections;
    double cellDistance;
    double gapX;
    double gapY;
};

#endif /* DECODEOPTIONS_H_ */
