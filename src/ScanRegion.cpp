/*
 * Used in JNI interface.
 */

#include "ScanRegion.h"
#include <stddef.h>

ScanRegion::ScanRegion(JNIEnv *env, jobject scanRegionObj) {
    jclass scanSettingsJavaClass = env->GetObjectClass(scanRegionObj);

    // run the following command to obtain method signatures from a class.
    // javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.ScanRegion
    jmethodID getMethod = env->GetMethodID(scanSettingsJavaClass, "getLeft",
                                           "()D");
    point1.x = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);

    getMethod = env->GetMethodID(scanSettingsJavaClass, "getTop", "()D");
    point1.y = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);

    getMethod = env->GetMethodID(scanSettingsJavaClass, "getRight", "()D");
    point2.x = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);

    getMethod = env->GetMethodID(scanSettingsJavaClass, "getBottom", "()D");
    point2.y = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);
}
