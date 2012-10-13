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

    point1.x = env->CallDoubleMethod(scanRegionObj, getMethodGetPointX, 0);
    point1.y = env->CallDoubleMethod(scanRegionObj, getMethodGetPointY, 0);

    point2.x = env->CallDoubleMethod(scanRegionObj, getMethodGetPointX, 1);
    point2.y = env->CallDoubleMethod(scanRegionObj, getMethodGetPointY, 1);
}
