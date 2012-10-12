/*
 * Contains JNI code common to both MS Windows and Linux.
 */

#include "edu_ualberta_med_scannerconfig_dmscanlib_ScanLib.h"
#include "DmScanLib.h"
#include "ScanRegion.h"
#include "DmScanLibInternal.h"

#include <iostream>
#include <vector>

using namespace std;

jobject createScanResultObject(JNIEnv * env, int resultCode, int value) {
    jclass scanLibResultClass = env->FindClass(
                    "edu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult");

    // run the following command to obtain method signatures from a class.
    // javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.ScanLibResult
    jmethodID cons = env->GetMethodID(scanLibResultClass, "<init>",
                                      "(IILjava/lang/String;)V");

    string msg;
    getResultCodeMsg(resultCode, msg);

    jvalue data[3];
    data[0].i = resultCode;
    data[1].i = value;
    data[2].l = env->NewStringUTF(msg.c_str());

    return env->NewObjectA(scanLibResultClass, cons, data);
}

jobject createDecodeResultObject(JNIEnv * env, int resultCode,
                std::vector<std::tr1::shared_ptr<DecodedWell> > * wells) {
    jclass resultClass = env->FindClass(
                    "edu/ualberta/med/scannerconfig/dmscanlib/DecodeResult");

    // run the following command to obtain method signatures from a class.
    // javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.DecodeResult
    jmethodID cons = env->GetMethodID(resultClass, "<init>",
                                      "(IILjava/lang/String;)V");

    string msg;
    getResultCodeMsg(resultCode, msg);

    jvalue data[3];
    data[0].i = resultCode;
    data[1].i = 0;
    data[2].l = env->NewStringUTF(msg.c_str());

    jobject resultObj = env->NewObjectA(resultClass, cons, data);

    jmethodID setCellMethod = env->GetMethodID(resultClass, "addWell",
    		"(Ljava/lang/String;Ljava/lang/String;)V");

    if (wells != NULL) {
        for (unsigned i = 0, n = wells->size(); i < n; ++i) {
        	DecodedWell & well = *(*wells)[i];
            jvalue data[3];

            data[0].i = env->NewStringUTF(well.getLabel().c_str());
            data[1].l = env->NewStringUTF(well.getMessage().c_str());

            env->CallObjectMethodA(resultObj, setCellMethod, data);
        }
    }

    return resultObj;
}



/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    slDecodeImage
 * Signature: (JJLjava/lang/String;DJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_decodeImage(
		JNIEnv * env, jobject, jlong, jstring, jobject, jobjectArray) {

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

    DmScanLib dmScanLib(verbose);
    int result = dmScanLib.decodeImage(filename, scanGap, squareDev, edgeThresh,
    		corrections, cellDistance);

    std::vector<std::tr1::shared_ptr<WellDecoder> > * cells = NULL;
    if (result == SC_SUCCESS) {
        cells = &dmScanLib.getDecodedCells();
    }
    jobject resultObj = createDecodedWellObject(env, result, cells);
    env->ReleaseStringUTFChars(_filename, filename);
    return resultObj;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    decodeImage
 * Signature: (JLjava/lang/String;Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeOptions;[Ledu/ualberta/med/scannerconfig/dmscanlib/Well;)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_decodeImage(
		JNIEnv * env, jobject obj, jlong _verbose, jstring _filename,
		jobject _decodeOptions, jobjectArray _wellRects) {

    unsigned verbose = static_cast<unsigned>(_verbose);
    const char *filename = env->GetStringUTFChars(_filename, 0);

    DecodeOptions decodeOptions(env, _decodeOptions);

    jobject wellRectJavaObj;
    jsize numWells = env->GetArrayLength(wellRects);
    jclass wellRectJavaClass = NULL;
    jmethodID wellRectCornerXMethodID = NULL;
    jmethodID wellRectCornerYMethodID = NULL;

    std::vector<std::tr1::shared_ptr<WellRectangle> > wellRects;

    for (unsigned i = 0; i < numWells; ++i) {
    	wellRectJavaObj = env->GetObjectArrayElement(_wellRects, i);

    	if (wellRectJavaClass == NULL) {
    		wellRectJavaClass = env->GetObjectClass(wellRectJavaObj);
    	    wellRectCornerXMethodID = env->GetMethodID(wellRectJavaClass,
    	    		"getCornerX", "(I)D");
    	    if(env->ExceptionOccurred()) {
    	    	return NULL;
    	    }
    	    wellRectCornerYMethodID = env->GetMethodID(wellRectJavaClass,
    	    		"getCornerY", "(I)D");
    	    if(env->ExceptionOccurred()) {
    	    	return NULL;
    	    }
    	}

    	std::tr1::shared_ptr<WellRectangle> rect(new WellRectangle());

    	env->DeleteLocalRef(wellRectJavaObj);
    }

    DmScanLib dmScanLib(verbose);
}

