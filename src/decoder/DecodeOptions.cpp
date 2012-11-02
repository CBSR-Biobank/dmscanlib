/*
 * DecodeOptions.cpp
 *
 *  Created on: 2012-10-11
 *      Author: nelson
 */

#include "DecodeOptions.h"
#include <stddef.h>
#include <jni.h>

namespace dmscanlib {

DecodeOptions::DecodeOptions(double _scanGap, long _squareDev,
		long _edgeThresh, long _corrections, long _shrink) :
    scanGap(_scanGap),squareDev(_squareDev), edgeThresh(_edgeThresh),
    corrections(_corrections),shrink(_shrink)
{
}

DecodeOptions::~DecodeOptions() {
}

std::unique_ptr<DecodeOptions> DecodeOptions::getDecodeOptionsViaJni(
		JNIEnv *env, jobject decodeOptionsObj) {
    jclass decodeOptionsJavaClass = env->GetObjectClass(decodeOptionsObj);

    // run the following command to obtain method signatures from a class.
    // javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.ScanRegion
    jmethodID getMethod = env->GetMethodID(
    		decodeOptionsJavaClass, "getScanGap", "()D");
    if(env->ExceptionOccurred()) {
    	return NULL;
    }
    double scanGap= env->CallDoubleMethod(decodeOptionsObj, getMethod, NULL);

    getMethod = env->GetMethodID(decodeOptionsJavaClass, "getSquareDev", "()J");
    if(env->ExceptionOccurred()) {
    	return NULL;
    }
    long squareDev= static_cast<long>(env->CallLongMethod(decodeOptionsObj, getMethod, NULL));

    getMethod = env->GetMethodID(decodeOptionsJavaClass, "getEdgeThresh", "()J");
    if(env->ExceptionOccurred()) {
    	return NULL;
    }
    long edgeThresh= static_cast<long>(env->CallLongMethod(decodeOptionsObj, getMethod, NULL));

    getMethod = env->GetMethodID(decodeOptionsJavaClass, "getCorrections", "()J");
    if(env->ExceptionOccurred()) {
    	return NULL;
    }
    long corrections= static_cast<long>(env->CallLongMethod(decodeOptionsObj, getMethod, NULL));

    getMethod = env->GetMethodID(decodeOptionsJavaClass, "getShrink", "()J");
    if(env->ExceptionOccurred()) {
    	return NULL;
    }
    long shrink = static_cast<long>(env->CallLongMethod(decodeOptionsObj, getMethod, NULL));

    return std::unique_ptr<DecodeOptions>(new DecodeOptions(
    	    scanGap, squareDev, edgeThresh, corrections, shrink));
}

std::ostream & operator<<(std::ostream &os, const DecodeOptions & m) {
	os << " scanGap/" << m.scanGap
			<< " squareDev/" << m.squareDev
			<< " edgeThresh/" << m.edgeThresh
			<< " corrections/"	<< m.corrections
			<< " shrink/" << m.shrink;
	return os;
}

} /* namespace */

