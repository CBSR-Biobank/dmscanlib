/*
 * Contains the code used in the library when building it for Linux.
 */

#include "DmScanLibJni.h"
#include "DmScanLibJniInternal.h"
#include "DmScanLib.h"
#include "geometry.h"

#include <iostream>

namespace dmscanlib {

namespace jni {

void getResultCodeMsg(int resultCode, std::string & message) {
    switch (resultCode) {
    case SC_SUCCESS:
        message = "";
        break;
    case SC_TWAIN_UNAVAIL:
        message = "Operation not supported on your operating system.";
        break;
    case SC_INVALID_IMAGE:
        message = "invalid image.";
        break;
    case SC_INVALID_DPI:
    case SC_INCORRECT_DPI_SCANNED:
        message = "invalid image DPI.";
        break;
    case SC_INVALID_NOTHING_DECODED:
        message = "No datamatrix barcodes detected in the image.";
        break;
    case SC_INVALID_NOTHING_TO_DECODE:
        message = "No wells to decode.";
		break;
    case SC_FAIL:
    default:
        message = "undefined error";
        break;
    }
}

} /* namespace */

} /* namespace */

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slIsTwainAvailable
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_isTwainAvailable(
                JNIEnv * env, jobject obj) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slSelectSourceAsDefault
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_selectSourceAsDefault(
                JNIEnv * env, jobject obj) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slGetScannerCapability
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_getScannerCapability(
                JNIEnv * env, jobject obj) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slScanImage
 * Signature: (JJIIDDDDLjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanImage(
                JNIEnv * env, jobject obj, jlong _verbose, jlong _dpi,
                jint _brightness, jint _contrast, jobject _region,
                jstring _filename) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slScanFlatbed
 * Signature: (JJIILjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanFlatbed(
                JNIEnv * env, jobject obj, jlong _verbose, jlong _dpi,
                jint _brightness, jint _contrast, jstring _filename) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    scanAndDecode
 * Signature: (JJIILedu/ualberta/med/scannerconfig/dmscanlib/BoundingBox;Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeOptions;[Ledu/ualberta/med/scannerconfig/dmscanlib/WellRectangle;)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanAndDecode
	(JNIEnv * env, jobject obj, jlong _verbose, jlong _dpi, jint _brightness, jint _contrast,
			jobject _region, jobject _decodeOptions, jobjectArray _wellRects) {
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_FAIL, dmscanlib::SC_FAIL);
}
