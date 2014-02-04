/*
 * Contains the code used in the library when building it for MS Windows.
 */

#define _CRT_SECURE_NO_DEPRECATE

#include "jni/DmScanLibJni.h"
#include "jni/DmScanLibJniInternal.h"
#include "DmScanLib.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellDecoder.h"

#include <iostream>

namespace dmscanlib {

namespace jni {

void getResultCodeMsg(int resultCode, std::string & message) {
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
    case SC_INVALID_NOTHING_DECODED:
        message = "no datamatrix barcodes could be decoded from the image";
		break;
	case SC_INVALID_IMAGE:
		message = "invalid image scanned";
		break;
    case SC_INVALID_NOTHING_TO_DECODE:
        message = "no wells to decode";
		break;
	case SC_INCORRECT_DPI_SCANNED:
		message = "incorrect DPI on scanned image";
		break;
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
    dmscanlib::DmScanLib dmScanLib;
    int result = dmScanLib.isTwainAvailable();
    return dmscanlib::jni::createScanResultObject(env, result, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slSelectSourceAsDefault
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_selectSourceAsDefault(
                JNIEnv * env, jobject obj) {
    dmscanlib::DmScanLib dmScanLib;
    int result = dmScanLib.selectSourceAsDefault();
    return dmscanlib::jni::createScanResultObject(env, result, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slGetScannerCapability
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_getScannerCapability(
                JNIEnv * env, jobject obj) {
    dmscanlib::DmScanLib dmScanLib;
    int result = dmScanLib.getScannerCapability();
    return dmscanlib::jni::createScanResultObject(env, dmscanlib::SC_SUCCESS, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slScanImage
 * Signature: (JJIIDDDDLjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanImage(
                JNIEnv * env, 
				jobject obj, 
				jlong _verbose, 
				jlong _dpi,
                jint _brightness, 
				jint _contrast, 
				jdouble x, 
				jdouble y, 
				jdouble width, 
				jdouble height, 
                jstring _filename) {

    if ((_dpi == 0)	|| (_filename == 0)) {
		return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_FAIL);
	}

    unsigned verbose = static_cast<unsigned>(_verbose);
    unsigned dpi = static_cast<unsigned>(_dpi);
    unsigned brightness = static_cast<unsigned>(_brightness);
    unsigned contrast = static_cast<unsigned>(_contrast);
    const char *filename = env->GetStringUTFChars(_filename, 0);

    dmscanlib::DmScanLib dmScanLib(verbose);
    int result = dmScanLib.scanImage(
		dpi, 
		brightness, 
		contrast, 
		static_cast<float>(x),
		static_cast<float>(y),
		static_cast<float>(width),
		static_cast<float>(height),
		filename);
    jobject resultObj = dmscanlib::jni::createScanResultObject(env, result, result);
    env->ReleaseStringUTFChars(_filename, filename);
    return resultObj;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slScanFlatbed
 * Signature: (JJIILjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanFlatbed(
                JNIEnv * env, 
				jobject obj, 
				jlong _verbose, 
				jlong _dpi,
                jint _brightness, 
				jint _contrast, 
				jstring _filename) {

    if ((_dpi == 0)	|| (_filename == 0)) {
		return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_FAIL);
	}

    unsigned verbose = static_cast<unsigned>(_verbose);
    unsigned dpi = static_cast<unsigned>(_dpi);
    unsigned brightness = static_cast<unsigned>(_brightness);
    unsigned contrast = static_cast<unsigned>(_contrast);
    const char *filename = env->GetStringUTFChars(_filename, 0);

    dmscanlib::DmScanLib dmScanLib(verbose);
    int result = dmScanLib.scanFlatbed(dpi, brightness, contrast, filename);
    jobject resultObj = dmscanlib::jni::createScanResultObject(env, result, result);
    env->ReleaseStringUTFChars(_filename, filename);
    return resultObj;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slDecodePlate
 * Signature: (JJIIJDDDDDJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanAndDecode
  (JNIEnv * env, 
  jobject obj, 
  jlong _verbose, 
  jlong _dpi,
  jint _brightness,
  jint _contrast, 
  jdouble x, 
  jdouble y, 
  jdouble width, 
  jdouble height, 
  jobject _decodeOptions,
  jobjectArray _wellRects) {

    if ((_dpi == 0)	|| (_decodeOptions == 0) || (_wellRects == 0)) {
		return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_FAIL);
	}

    dmscanlib::DmScanLib::configLogging(static_cast<unsigned>(_verbose), false);

    unsigned dpi = static_cast<unsigned>(_dpi);
    unsigned brightness = static_cast<unsigned>(_brightness);
    unsigned contrast = static_cast<unsigned>(_contrast);
    std::vector<std::unique_ptr<const dmscanlib::WellRectangle> > wellRects;

    std::unique_ptr<dmscanlib::DecodeOptions> decodeOptions = 
		dmscanlib::DecodeOptions::getDecodeOptionsViaJni(env, _decodeOptions);

	jsize numWells = env->GetArrayLength(_wellRects);
	int result = dmscanlib::jni::getWellRectangles(env, numWells, _wellRects, wellRects);

	if (result == 0) {
		// got an exception when converting from JNI
		return NULL;
	} else if ((result != 1) || (wellRects.size() == 0)) {
    	// invalid rects or zero rects passed from java
    	return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_INVALID_NOTHING_TO_DECODE);
	}

    dmscanlib::DmScanLib dmScanLib(0);
    result = dmScanLib.scanAndDecode(
		dpi, 
		brightness, 
		contrast, 
		static_cast<float>(x),
		static_cast<float>(y),
		static_cast<float>(width),
		static_cast<float>(height),
		*decodeOptions, 
		wellRects);

	if (result == dmscanlib::SC_SUCCESS) {
		return dmscanlib::jni::createDecodeResultObject(env,result, dmScanLib.getDecodedWells());
	}
	return dmscanlib::jni::createDecodeResultObject(env, result);
}


