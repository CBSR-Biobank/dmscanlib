/*
 * Contains JNI code common to both MS Windows and Linux.
 */

#include "edu_ualberta_med_scannerconfig_dmscanlib_ScanLib.h"
#include "DmScanLib.h"
#include "ScanRegion.h"
#include "DecodeOptions.h"
#include "DecodedWell.h"

#include <iostream>
#include <vector>
#include <glog/logging.h>

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

            data[0].l = env->NewStringUTF(well.getLabel().c_str());
            data[1].l = env->NewStringUTF(well.getMessage().c_str());

            env->CallObjectMethodA(resultObj, setCellMethod, data);
        }
    }

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

    vector<std::tr1::shared_ptr<WellRectangle<double>  > > wellRects;

    jobject wellRectJavaObj;
    jclass wellRectJavaClass = NULL;
    jmethodID wellRectGetLabelMethodID = NULL;
    jmethodID wellRectGetCornerXMethodID = NULL;
    jmethodID wellRectGetCornerYMethodID = NULL;

    jsize numWells = env->GetArrayLength(_wellRects);

	// TODO check for max well rectangle objects

    DmScanLib::configLogging(3, false);

    for (int i = 0; i < static_cast<int>(numWells); ++i) {
    	wellRectJavaObj = env->GetObjectArrayElement(_wellRects, i);

    	if (wellRectJavaClass == NULL) {
    		wellRectJavaClass = env->GetObjectClass(wellRectJavaObj);

    		wellRectGetLabelMethodID = env->GetMethodID(wellRectJavaClass,
    	    		"getLabel", "()Ljava/lang/String;");
    	    if(env->ExceptionOccurred()) {
    	    	return NULL;
    	    }

    	    wellRectGetCornerXMethodID = env->GetMethodID(wellRectJavaClass,
    	    		"getCornerX", "(I)D");
    	    if(env->ExceptionOccurred()) {
    	    	return NULL;
    	    }

    	    wellRectGetCornerYMethodID = env->GetMethodID(wellRectJavaClass,
    	    		"getCornerY", "(I)D");
    	    if(env->ExceptionOccurred()) {
    	    	return NULL;
    	    }
    	}

    	jobject labelJobj = env->CallObjectMethod(wellRectJavaObj, wellRectGetLabelMethodID);
    	const char * label = env->GetStringUTFChars((jstring) labelJobj, NULL);

    	double x1 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerXMethodID, 0);
    	double y1 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerYMethodID, 0);

    	double x2 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerXMethodID, 1);
    	double y2 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerYMethodID, 1);

    	double x3 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerXMethodID, 2);
    	double y3 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerYMethodID, 2);

    	double x4 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerXMethodID, 3);
    	double y4 = env->CallDoubleMethod(wellRectJavaObj, wellRectGetCornerYMethodID, 3);

    	std::tr1::shared_ptr<WellRectangle<double> > rect(
    			new WellRectangle<double>(label, x1, y1, x2, y2, x3, y3, x4, y4));

    	VLOG(3) << *rect;

    	env->ReleaseStringUTFChars((jstring) labelJobj, label);
    	env->DeleteLocalRef(wellRectJavaObj);
    }

    DmScanLib dmScanLib(verbose);
    dmScanLib.decodeImage(filename, decodeOptions, wellRects);
    env->ReleaseStringUTFChars(_filename, filename);

    // TODO create result object
    return NULL;
}

