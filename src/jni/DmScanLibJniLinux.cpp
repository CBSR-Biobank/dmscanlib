/*
 * Contains the code used in the library when building it for Linux.
 */

#include "DmScanLibJni.h"
#include "DmScanLibJniInternal.h"
#include "DmScanLib.h"
#include "geometry.h"

#include <iostream>

using namespace dmscanlib;


void getResultCodeMsg(int resultCode, std::string & message) {
    switch (resultCode) {
    case SC_SUCCESS:
        message = "";
        break;
    case SC_FAIL:
    case SC_TWAIN_UNAVAIL:
    case SC_INVALID_DPI:
    case SC_INVALID_IMAGE:
    case SC_INCORRECT_DPI_SCANNED:
        message = "operation not supported on your operating system";
        break;
    case SC_INVALID_NOTHING_DECODED:
        message = "no datamatrix barcodes could be decoded from the image";
        break;
    case SC_INVALID_VALUE:
        message = "invalid value specified";
        break;
    default:
        message = "undefined error";
        break;
    }
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slIsTwainAvailable
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_isTwainAvailable(
                JNIEnv * env, jobject obj) {
    return createScanResultObject(env, SC_FAIL, SC_FAIL);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slSelectSourceAsDefault
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_selectSourceAsDefault(
                JNIEnv * env, jobject obj) {
    return createScanResultObject(env, SC_FAIL, SC_FAIL);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slGetScannerCapability
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_getScannerCapability(
                JNIEnv * env, jobject obj) {
    return createScanResultObject(env, SC_FAIL, SC_FAIL);
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
    return createScanResultObject(env, SC_FAIL, SC_FAIL);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slScanFlatbed
 * Signature: (JJIILjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanFlatbed(
                JNIEnv * env, jobject obj, jlong _verbose, jlong _dpi,
                jint _brightness, jint _contrast, jstring _filename) {
    return createScanResultObject(env, SC_FAIL, SC_FAIL);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slDecodePlate
 * Signature: (JJIIJDDDDDJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodedWell;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_decodePlate(
                JNIEnv * env, jobject obj, jlong _verbose, jlong _dpi,
                jint brightness, jint contrast, jlong _plateNum,
                jobject _region, jdouble scanGap, jlong _squareDev,
                jlong _edgeThresh, jlong _corrections, jdouble cellDistance,
                jdouble gapX, jdouble gapY, jlong _profileA, jlong _profileB,
                jlong _profileC, jlong _orientation) {
    return createScanResultObject(env, SC_FAIL, SC_FAIL);
}
