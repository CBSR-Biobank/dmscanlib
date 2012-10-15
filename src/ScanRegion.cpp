/*
 * Used in JNI interface.
 */

#include "ScanRegion.h"
#include <stddef.h>

ScanRegion::ScanRegion(JNIEnv *env, jobject scanRegionObj) {
    jclass scanSettingsJavaClass = env->GetObjectClass(scanRegionObj);

    // run the following command to obtain method signatures from a class.
    // javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.ScanRegion
    jmethodID getMethodGetPointX = env->GetMethodID(scanSettingsJavaClass,
    		"getPointX", "(I)Ljava/lang/Double");
    if(env->ExceptionOccurred()) {
    	return;
    }

    jmethodID getMethodGetPointY = env->GetMethodID(scanSettingsJavaClass,
    		"getPointY", "(I)Ljava/lang/Double");
    if(env->ExceptionOccurred()) {
    	return;
    }

    boundingBox.points[0].x = env->CallDoubleMethod(scanRegionObj, getMethodGetPointX, 0);
    boundingBox.points[0].y = env->CallDoubleMethod(scanRegionObj, getMethodGetPointY, 0);

    boundingBox.points[1].x = env->CallDoubleMethod(scanRegionObj, getMethodGetPointX, 1);
    boundingBox.points[1].y = env->CallDoubleMethod(scanRegionObj, getMethodGetPointY, 1);
}
