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

#include "geometry.h"
#include "decoder/WellRectangle.h"

#include <jni.h>
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace dmscanlib {

class WellDecoder;

namespace jni {

void getResultCodeMsg(int resultCode, std::string & message);

jobject createScanResultObject(JNIEnv * env, int resultCode, int value);

jobject createDecodeResultObject(JNIEnv * env, int resultCode);

jobject createDecodeResultObject(JNIEnv * env, int resultCode,
        const std::map<std::string, const WellDecoder *> & wellDecoders);

std::unique_ptr<BoundingBox<float> > getBoundingBox(
        JNIEnv *env, jobject bboxJavaObj);

std::unique_ptr<ScanRegion<float> > getScanRegion(
        JNIEnv *env, jobject regionJavaObj);

int getWellRectangles(JNIEnv *env, jsize numWells, jobjectArray _wellRects,
        std::vector<std::unique_ptr<const WellRectangle> > & wellRects);

} /* namespace */

} /* namespace */

#endif /* DMSCANLIBJNIINTERNAL_H_ */
