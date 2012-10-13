/*
 * Contains the code used in the library when building it for MS Windows.
 */

#include "edu_ualberta_med_scannerconfig_dmscanlib_ScanLib.h"
#include "DmScanLib.h"
#include "ScanRegion.h"
#include "DmScanLibInternal.h"

#include <iostream>

using namespace std;

void getResultCodeMsg(int resultCode, string & message) {
	switch (resultCode) {
	case SC_SUCCESS:
		message = "";
		break;
	case SC_FAIL:
		message = "operation failed";
		break;
	case SC_TWAIN_UNAVAIL:
		message = "twain driver unavailable";
		break;
	case SC_INVALID_DPI:
		message = "invalid DPI specified";
		break;
	case SC_INVALID_PLATE_NUM:
		message = "invalid plate number specified";
		break;
	case SC_INVALID_VALUE:
		message = "invalid value specified";
		break;
	case SC_INVALID_IMAGE:
		message = "invalid image scanned";
		break;
	case SC_INCORRECT_DPI_SCANNED:
		message = "incorrect DPI on scanned image";
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
    DmScanLib dmScanLib(0);
    int result = dmScanLib.isTwainAvailable();
    return createScanResultObject(env, result, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slSelectSourceAsDefault
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_selectSourceAsDefault(
                JNIEnv * env, jobject obj) {
    DmScanLib dmScanLib(0);
    int result = dmScanLib.selectSourceAsDefault();
    return createScanResultObject(env, result, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slGetScannerCapability
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_getScannerCapability(
                JNIEnv * env, jobject obj) {
    DmScanLib dmScanLib(0);
    int result = dmScanLib.getScannerCapability();
    return createScanResultObject(env, SC_SUCCESS, result);
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

    unsigned verbose = static_cast<unsigned>(_verbose);
    unsigned dpi = static_cast<unsigned>(_dpi);
    unsigned brightness = static_cast<unsigned>(_brightness);
    unsigned contrast = static_cast<unsigned>(_contrast);
    const char *filename = env->GetStringUTFChars(_filename, 0);
    ScanRegion region(env, _region);

    DmScanLib dmScanLib(verbose);
    int result = dmScanLib.scanImage(dpi, brightness, contrast,
    		region.point1.x, region.point1.y, region.point2.x, region.point2.y,
    		filename);
    jobject resultObj = createScanResultObject(env, result, result);
    env->ReleaseStringUTFChars(_filename, filename);
    return resultObj;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slScanFlatbed
 * Signature: (JJIILjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanFlatbed(
                JNIEnv * env, jobject obj, jlong _verbose, jlong _dpi,
                jint _brightness, jint _contrast, jstring _filename) {

    unsigned verbose = static_cast<unsigned>(_verbose);
    unsigned dpi = static_cast<unsigned>(_dpi);
    unsigned brightness = static_cast<unsigned>(_brightness);
    unsigned contrast = static_cast<unsigned>(_contrast);
    const char *filename = env->GetStringUTFChars(_filename, 0);

    DmScanLib dmScanLib(verbose);
    int result = dmScanLib.scanFlatbed(dpi, brightness, contrast, filename);
    jobject resultObj = createScanResultObject(env, result, result);
    env->ReleaseStringUTFChars(_filename, filename);
    return resultObj;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slDecodePlate
 * Signature: (JJIIJDDDDDJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_decodePlate(
                JNIEnv * env, jobject obj, jlong _verbose, jlong _dpi,
                jint brightness, jint contrast, jlong _plateNum,
                jobject _region, jdouble scanGap, jlong _squareDev,
                jlong _edgeThresh, jlong _corrections, jdouble cellDistance,
                jdouble gapX, jdouble gapY, jlong _profileA, jlong _profileB,
                jlong _profileC, jlong _orientation) {

    unsigned verbose = static_cast<unsigned>(_verbose);
    unsigned dpi = static_cast<unsigned>(_dpi);
    unsigned plateNum = static_cast<unsigned>(_plateNum);
    unsigned squareDev = static_cast<unsigned>(_squareDev);
    unsigned edgeThresh = static_cast<unsigned>(_edgeThresh);
    unsigned corrections = static_cast<unsigned>(_corrections);
    unsigned profileA = static_cast<unsigned>(_profileA);
    unsigned profileB = static_cast<unsigned>(_profileB);
    unsigned profileC = static_cast<unsigned>(_profileC);
    unsigned orientation = static_cast<unsigned>(_orientation);

    ScanRegion region(env, _region);

    DmScanLib dmScanLib(verbose);
    int result = dmScanLib.scanAndDecode(dpi, brightness, contrast,
    		region.point1.x, region.point1.y, region.point2.x, region.point2.y,
    		scanGap, squareDev, edgeThresh, corrections, cellDistance);

    // TODO: needs implementation
    return NULL;
}


