#ifndef DMSCANLIBJNIINTERNAL_H_
#define DMSCANLIBJNIINTERNAL_H_

/*
 * DmScanLibJniInternal.h
 *
 *  Created on: 2012-10-15
 *      Author: nelson
 *
 *  This file not meant to be included from outside this directory.
 */

#include <jni.h>
#include <vector>
#include <string>

class WellDecoder;

void getResultCodeMsg(int resultCode, std::string & message);

jobject createScanResultObject(JNIEnv * env, int resultCode, int value);

jobject createDecodeResultObject(JNIEnv * env, int resultCode,
		const std::vector<WellDecoder *> & wellDecoders);


#endif /* DMSCANLIBJNIINTERNAL_H_ */
