#include <edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl.h>
#include "DmScanLib.h"

jobject createScanResultObject(JNIEnv * env, int resultCode, int value) {
	const char * message = NULL;

	jclass scanLibResultClass = env->FindClass("ScanLibResult");

	// run the following command to obtain method signatures from a class.
	// javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.ScanLibResult
	jmethodID cons = env->GetMethodID(scanLibResultClass, "<init>",
			"(IILjava/lang/String;)V");

	switch (resultCode) {
	case SC_SUCCESS:
		message = NULL;
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

	jvalue data[4];
	data[0].l = resultCode;
	data[1].l = value;
	data[2].i = env->NewStringUTF(message);

	jobject scanLibResult = env->NewObjectA(scanLibResultClass, cons, data);

}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slIsTwainAvailable
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slIsTwainAvailable
  (JNIEnv *, jobject) {

	int result = slIsTwainAvailable();

	jobject returnObject = createScanResultObject(env, result, result);

	return returnObject;

}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slSelectSourceAsDefault
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slSelectSourceAsDefault
  (JNIEnv *, jobject) {

}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slGetScannerCapability
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slGetScannerCapability
  (JNIEnv *, jobject) {

}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slScanImage
 * Signature: (JJIIDDDDLjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slScanImage
  (JNIEnv *, jobject, jlong, jlong, jint, jint, jdouble, jdouble, jdouble, jdouble, jstring) {

}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slScanFlatbed
 * Signature: (JJIILjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slScanFlatbed
  (JNIEnv *, jobject, jlong, jlong, jint, jint, jstring) {

}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slDecodePlate
 * Signature: (JJIIJDDDDDJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slDecodePlate
  (JNIEnv *, jobject, jlong, jlong, jint, jint, jlong, jdouble, jdouble, jdouble, jdouble, jdouble, jlong, jlong, jlong, jdouble, jdouble, jdouble, jlong, jlong, jlong, jlong) {

}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slDecodeImage
 * Signature: (JJLjava/lang/String;DJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slDecodeImage
  (JNIEnv *, jobject, jlong, jlong, jstring, jdouble, jlong, jlong, jlong, jdouble, jdouble, jdouble, jlong, jlong, jlong, jlong) {

}

