/*
 * Used in JNI interface.
 */

#include "BoundingBox.h"
#include <stddef.h>

BoundingBox::BoundingBox(JNIEnv *env, jobject scanRegionObj) {
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

    boundingBox = new BoundingBox(
    		env->CallDoubleMethod(scanRegionObj, getMethodGetPointX, 0),
    		env->CallDoubleMethod(scanRegionObj, getMethodGetPointY, 0),
    		env->CallDoubleMethod(scanRegionObj, getMethodGetPointX, 1),
    		env->CallDoubleMethod(scanRegionObj, getMethodGetPointY, 1));
}
