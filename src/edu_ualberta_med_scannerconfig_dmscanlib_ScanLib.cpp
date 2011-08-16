#include <edu_ualberta_med_scannerconfig_dmscanlib_ScanLib.h>
#include "DmScanLib.h"
#include "DmScanLibInternal.h"
#include "DecodeInfo.h"

#include <iostream>

using namespace std;

class ScanRegion {
public:
	ScanRegion(JNIEnv *env, jobject scanRegionObj) {
		jclass scanSettingsJavaClass = env->GetObjectClass(scanRegionObj);

		// run the following command to obtain method signatures from a class.
		// javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.ScanRegion
		jmethodID getMethod = env->GetMethodID(scanSettingsJavaClass, "getLeft", "()D");
		left = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);

		getMethod = env->GetMethodID(scanSettingsJavaClass, "getTop", "()D");
		top = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);

		getMethod = env->GetMethodID(scanSettingsJavaClass, "getRight", "()D");
		right = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);

		getMethod = env->GetMethodID(scanSettingsJavaClass, "getBottom", "()D");
		bottom = env->CallDoubleMethod(scanRegionObj, getMethod, NULL);
    }

	double left;
	double top;
	double right;
	double bottom;

private:
};

const char * getResultCodeMsg(int resultCode) {
	const char * message = NULL;

#ifdef WIN32
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
#else
	switch (resultCode) {
	case SC_SUCCESS:
		message = NULL;
		break;
	case SC_FAIL:
	case SC_TWAIN_UNAVAIL:
	case SC_INVALID_DPI:
	case SC_INVALID_IMAGE:
	case SC_INCORRECT_DPI_SCANNED:
		message = "operation not supported on your operating system";
		break;
	case SC_INVALID_PLATE_NUM:
		message = "invalid plate number specified";
		break;
	case SC_INVALID_VALUE:
		message = "invalid value specified";
		break;
	default:
		message = "undefined error";
		break;
	}
#endif
	return message;
}

jobject createScanResultObject(JNIEnv * env, int resultCode, int value) {
	jclass scanLibResultClass = env->FindClass(
			"edu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult");

	// run the following command to obtain method signatures from a class.
	// javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.ScanLibResult
	jmethodID cons = env->GetMethodID(scanLibResultClass, "<init>",
			"(IILjava/lang/String;)V");

	jvalue data[3];
	data[0].i = resultCode;
	data[1].i = value;
	data[2].l = env->NewStringUTF(getResultCodeMsg(resultCode));

	return env->NewObjectA(scanLibResultClass, cons, data);
}

jobject createDecodeResultObject(JNIEnv * env, int resultCode,
		vector<DecodeInfo *> * barcodes) {
	jclass resultClass = env->FindClass(
			"edu/ualberta/med/scannerconfig/dmscanlib/DecodeResult");

	// run the following command to obtain method signatures from a class.
	// javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.DecodeResult
	jmethodID cons = env->GetMethodID(resultClass, "<init>",
			"(IILjava/lang/String;)V");

	jvalue data[3];
	data[0].i = resultCode;
	data[1].i = 0;
	data[2].l = env->NewStringUTF(getResultCodeMsg(resultCode));

	jobject resultObj = env->NewObjectA(resultClass, cons, data);

	jmethodID setCellMethod = env->GetMethodID(resultClass, "setCell",
			"(IILjava/lang/String;)V");

	if (barcodes != NULL) {
		for (unsigned i = 0, n = barcodes->size(); i < n; ++i) {
			DecodeInfo & info = *(*barcodes)[i];
			jvalue data[3];

			// TODO: convert to PalletCell
			//data[0].i = info.getRow();
			//data[1].i = info.getCol();

			data[2].l = env->NewStringUTF(info.getMsg().c_str());

			env->CallObjectMethodA(resultObj, setCellMethod, data);
		}
	}

	return resultObj;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slIsTwainAvailable
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_isTwainAvailable(
		JNIEnv * env, jobject obj) {
	DmScanLib dmScanLib;
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
	DmScanLib dmScanLib;
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
	DmScanLib dmScanLib;
	int result = dmScanLib.getScannerCapability();
	return createScanResultObject(env, SC_SUCCESS, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slScanImage
 * Signature: (JJIIDDDDLjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_scanImage(
		JNIEnv * env, jobject obj, jlong _verbose, jlong _dpi, jint _brightness,
		jint _contrast, jobject _region, jstring _filename) {

	unsigned verbose = static_cast<unsigned>(_verbose);
	unsigned dpi = static_cast<unsigned>(_dpi);
	unsigned brightness = static_cast<unsigned>(_brightness);
	unsigned contrast = static_cast<unsigned>(_contrast);
	const char *filename = env->GetStringUTFChars(_filename, 0);
	ScanRegion region(env, _region);

	DmScanLib dmScanLib;
	int result = dmScanLib.scanImage(verbose, dpi, brightness, contrast,
		region.left, region.top, region.right, region.bottom, filename);
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
		JNIEnv * env, jobject obj, jlong _verbose, jlong _dpi, jint _brightness,
		jint _contrast, jstring _filename) {

	unsigned verbose = static_cast<unsigned>(_verbose);
	unsigned dpi = static_cast<unsigned>(_dpi);
	unsigned brightness = static_cast<unsigned>(_brightness);
	unsigned contrast = static_cast<unsigned>(_contrast);
	const char *filename = env->GetStringUTFChars(_filename, 0);

	DmScanLib dmScanLib;
	int result = dmScanLib.scanFlatbed(verbose, dpi, brightness, contrast, filename);
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
		JNIEnv * env, jobject obj, jlong _verbose, jlong _dpi, jint brightness,
		jint contrast, jlong _plateNum, jobject _region, jdouble scanGap,
		jlong _squareDev, jlong _edgeThresh, jlong _corrections,
		jdouble cellDistance, jdouble gapX, jdouble gapY, jlong _profileA,
		jlong _profileB, jlong _profileC, jlong _orientation) {

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

	DmScanLib dmScanLib;
	vector<DecodeInfo *> * barcodes = NULL;
	int result = dmScanLib.decodePlate(verbose, dpi, brightness, contrast, plateNum,
		region.left, region.top, region.right, region.bottom, scanGap,
			squareDev, edgeThresh, corrections, cellDistance, gapX, gapY,
			profileA, profileB, profileC, orientation);

	if (result == SC_SUCCESS) {
	    // TODO: get pallet cells
		//barcodes = &dmScanLib.getBarcodes();
	}
	return createDecodeResultObject(env, result, barcodes);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slDecodeImage
 * Signature: (JJLjava/lang/String;DJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_decodeImage(
		JNIEnv * env, jobject obj, jlong _verbose, jlong _plateNum,
		jstring _filename, jdouble scanGap, jlong _squareDev, jlong _edgeThresh,
		jlong _corrections, jdouble cellDistance, jdouble gapX, jdouble gapY,
		jlong _profileA, jlong _profileB, jlong _profileC, jlong _orientation) {

	unsigned verbose = static_cast<unsigned>(_verbose);
	unsigned plateNum = static_cast<unsigned>(_plateNum);
	unsigned squareDev = static_cast<unsigned>(_squareDev);
	unsigned edgeThresh = static_cast<unsigned>(_edgeThresh);
	unsigned corrections = static_cast<unsigned>(_corrections);
	unsigned profileA = static_cast<unsigned>(_profileA);
	unsigned profileB = static_cast<unsigned>(_profileB);
	unsigned profileC = static_cast<unsigned>(_profileC);
	unsigned orientation = static_cast<unsigned>(_orientation);
	const char *filename = env->GetStringUTFChars(_filename, 0);

	DmScanLib dmScanLib;
	vector<DecodeInfo *> * barcodes = NULL;
	int result = dmScanLib.decodeImage(verbose, plateNum, filename, scanGap,
			squareDev, edgeThresh, corrections, cellDistance, gapX, gapY,
			profileA, profileB, profileC, orientation);

	if (result == SC_SUCCESS) {
        // TODO: get pallet cells
		//barcodes = &dmScanLib.getBarcodes();
	}
	jobject resultObj = createDecodeResultObject(env, result, barcodes);
	env->ReleaseStringUTFChars(_filename, filename);
	return resultObj;
}

