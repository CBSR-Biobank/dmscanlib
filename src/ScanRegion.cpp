/*
 * Used in JNI interface.
 */

#include "ScanRegion.h"
#include "stddef.h"

ScanRegion::ScanRegion(JNIEnv *env, jobject scanRegionObj) {
    jclass scanSettingsJavaClass = env->GetObjectClass(scanRegionObj);

    // run the following command to obtain method signatures from a class.
    // javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.ScanRegion
    jmethodID getMethod = env->GetMethodID(scanSettingsJavaClass, "getLeft",
                                           "()D");
    left = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);

    getMethod = env->GetMethodID(scanSettingsJavaClass, "getTop", "()D");
    top = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);

    getMethod = env->GetMethodID(scanSettingsJavaClass, "getRight", "()D");
    right = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);

    getMethod = env->GetMethodID(scanSettingsJavaClass, "getBottom", "()D");
    bottom = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);
}
