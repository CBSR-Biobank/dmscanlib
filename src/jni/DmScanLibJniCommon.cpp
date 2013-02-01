/*
 * Contains JNI code common to both MS Windows and Linux.
 */

#include "DmScanLibJni.h"
#include "DmScanLibJniInternal.h"
#include "DmScanLib.h"
#include "geometry.h"
#include "decoder/DecodeOptions.h"
#include "decoder/WellDecoder.h"

#include <iostream>
#include <map>
#include <memory>
#include <glog/logging.h>

namespace dmscanlib {

namespace jni {

jobject createScanResultObject(JNIEnv * env, int resultCode, int value) {
    jclass scanLibResultClass = env->FindClass(
                    "edu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult");

    // run the following command to obtain method signatures from a class.
    // javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.ScanLibResult
    jmethodID cons = env->GetMethodID(scanLibResultClass, "<init>",
                                      "(IILjava/lang/String;)V");

    std::string msg;
    getResultCodeMsg(resultCode, msg);

    jvalue data[3];
    data[0].i = resultCode;
    data[1].i = value;
    data[2].l = env->NewStringUTF(msg.c_str());

    return env->NewObjectA(scanLibResultClass, cons, data);
}

jobject createDecodeResultObject(JNIEnv * env, int resultCode) {
    jclass resultClass = env->FindClass(
                    "edu/ualberta/med/scannerconfig/dmscanlib/DecodeResult");

    // run the following command to obtain method signatures from a class.
    // javap -s -p edu.ualberta.med.scannerconfig.dmscanlib.DecodeResult
    jmethodID cons = env->GetMethodID(resultClass, "<init>",
                                      "(IILjava/lang/String;)V");

    std::string msg;
    getResultCodeMsg(resultCode, msg);

    jvalue data[3];
    data[0].i = resultCode;
    data[1].i = 0;
    data[2].l = env->NewStringUTF(msg.c_str());

    return env->NewObjectA(resultClass, cons, data);
}

jobject createDecodeResultObject(JNIEnv * env, int resultCode,
                const std::map<std::string, const dmscanlib::WellDecoder *> & wellDecoders) {
    jobject resultObj = createDecodeResultObject(env, resultCode);

    jclass resultClass = env->FindClass(
                    "edu/ualberta/med/scannerconfig/dmscanlib/DecodeResult");

    if (wellDecoders.size() > 0) {
        jmethodID setCellMethod = env->GetMethodID(resultClass, "addWell",
        		"(Ljava/lang/String;Ljava/lang/String;)V");

    	for (std::map<std::string, const dmscanlib::WellDecoder *>::const_iterator ii = wellDecoders.begin();
    			ii != wellDecoders.end(); ++ii) {
        	const dmscanlib::WellDecoder & wellDecoder = *(ii->second);
            jvalue data[3];

			VLOG(5) << wellDecoder;

            data[0].l = env->NewStringUTF(wellDecoder.getLabel().c_str());
            data[1].l = env->NewStringUTF(wellDecoder.getMessage().c_str());

            env->CallObjectMethodA(resultObj, setCellMethod, data);
        }

		VLOG(1) << "wells decoded: " << wellDecoders.size();
    }
	
    return resultObj;
}

int getWellRectangles(JNIEnv *env, jsize numWells, jobjectArray _wellRects,
					   std::vector<std::unique_ptr<const WellRectangle<double> > > & wellRects) {
    jobject wellRectJavaObj;
    jclass wellRectJavaClass = NULL;
    jmethodID wellRectGetLabelMethodID = NULL;
    jmethodID wellRectGetCornerXMethodID = NULL;
    jmethodID wellRectGetCornerYMethodID = NULL;

	VLOG(5) << "decodeImage: numWells/" << numWells;

	// TODO check for max well rectangle objects
    for (int i = 0; i < static_cast<int>(numWells); ++i) {
    	wellRectJavaObj = env->GetObjectArrayElement(_wellRects, i);

    	// if java object pointer is null, skip this array element
    	if (wellRectJavaObj == NULL) {
    		return 2;
    	}

    	if (wellRectJavaClass == NULL) {
    		wellRectJavaClass = env->GetObjectClass(wellRectJavaObj);

    		wellRectGetLabelMethodID = env->GetMethodID(wellRectJavaClass,
    	    		"getLabel", "()Ljava/lang/String;");
    	    if(env->ExceptionOccurred()) {
    	    	return 0;
    	    }

    	    wellRectGetCornerXMethodID = env->GetMethodID(wellRectJavaClass,
    	    		"getCornerX", "(I)D");
    	    if(env->ExceptionOccurred()) {
    	    	return 0;
    	    }

    	    wellRectGetCornerYMethodID = env->GetMethodID(wellRectJavaClass,
    	    		"getCornerY", "(I)D");
    	    if(env->ExceptionOccurred()) {
    	    	return 0;
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

    	Point<double> pt1(x1, y1);
    	Point<double> pt2(x2, y2);
    	Point<double> pt3(x3, y3);
    	Point<double> pt4(x4, y4);
	Rect<double> rect(pt1, pt2, pt3, pt4);

	std::unique_ptr<const WellRectangle<double> > wellRect(new WellRectangle<double>(label, rect));

    	VLOG(5) << *wellRect;

    	wellRects.push_back(std::move(wellRect));

    	env->ReleaseStringUTFChars((jstring) labelJobj, label);
    	env->DeleteLocalRef(wellRectJavaObj);
    }
	return 1;
}

std::unique_ptr<BoundingBox<double> > getBoundingBox(JNIEnv *env, jobject bboxJavaObj) {
	CHECK_NOTNULL(bboxJavaObj);
	jclass bboxJavaClass = env->GetObjectClass(bboxJavaObj);

	jmethodID getCornerXMethodID = env->GetMethodID(bboxJavaClass, "getCornerX", "(I)D");
    if(env->ExceptionOccurred()) {
      	return NULL;
    }

	jmethodID getCornerYMethodID = env->GetMethodID(bboxJavaClass, "getCornerY", "(I)D");
    if(env->ExceptionOccurred()) {
      	return NULL;
    }

	Point<double> pt0(
		env->CallDoubleMethod(bboxJavaObj, getCornerXMethodID, 0),
		env->CallDoubleMethod(bboxJavaObj, getCornerYMethodID, 0));

	Point<double> pt1(
		env->CallDoubleMethod(bboxJavaObj, getCornerXMethodID, 1),
		env->CallDoubleMethod(bboxJavaObj, getCornerYMethodID, 1));

	return std::unique_ptr<BoundingBox<double> >(new BoundingBox<double>(
		pt0, pt1));
}

std::unique_ptr<ScanRegion<double> > getScanRegion(JNIEnv *env, jobject regionJavaObj) {
	CHECK_NOTNULL(regionJavaObj);
	jclass regionJavaClass = env->GetObjectClass(regionJavaObj);

	jmethodID getCornerXMethodID = env->GetMethodID(regionJavaClass, "getCornerX", "(I)D");
    if(env->ExceptionOccurred()) {
      	return NULL;
    }

	jmethodID getCornerYMethodID = env->GetMethodID(regionJavaClass, "getCornerY", "(I)D");
    if(env->ExceptionOccurred()) {
      	return NULL;
    }

	Point<double> pt0(
		env->CallDoubleMethod(regionJavaObj, getCornerXMethodID, 0),
		env->CallDoubleMethod(regionJavaObj, getCornerYMethodID, 0));

	Point<double> pt1(
		env->CallDoubleMethod(regionJavaObj, getCornerXMethodID, 1),
		env->CallDoubleMethod(regionJavaObj, getCornerYMethodID, 1));

	return std::unique_ptr<ScanRegion<double> >(new ScanRegion<double>(
		pt0, pt1));
}

} /* namespace */

} /* namespace */


/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    decodeImage
 * Signature: (JLjava/lang/String;Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeOptions;[Ledu/ualberta/med/scannerconfig/dmscanlib/Well;)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_decodeImage(
		JNIEnv * env, jobject obj, jlong _verbose, jstring _filename,
		jobject _decodeOptions, jobjectArray _wellRects) {

	if ((_filename == 0) || (_decodeOptions == 0) || (_wellRects == 0)) {
		return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_FAIL);
	}

	dmscanlib::DmScanLib::configLogging(static_cast<unsigned>(_verbose), false);
    std::unique_ptr<dmscanlib::DecodeOptions> decodeOptions =
    		dmscanlib::DecodeOptions::getDecodeOptionsViaJni(env, _decodeOptions);
    std::vector<std::unique_ptr<const dmscanlib::WellRectangle<double>  > > wellRects;

    jsize numWells = env->GetArrayLength(_wellRects);
	int result = dmscanlib::jni::getWellRectangles(env, numWells, _wellRects, wellRects);

	if (result == 0) {
		// got an exception when converting from JNI
		return NULL;
	} else if ((result != 1) || (wellRects.size() == 0)) {
    	// invalid rects or zero rects passed from java
    	return dmscanlib::jni::createDecodeResultObject(env, dmscanlib::SC_INVALID_NOTHING_TO_DECODE);
	}

	dmscanlib::DmScanLib dmScanLib(1);

	const char *filename = env->GetStringUTFChars(_filename, 0);
	result = dmScanLib.decodeImageWells(filename, *decodeOptions, wellRects);
	env->ReleaseStringUTFChars(_filename, filename);

	if (result == dmscanlib::SC_SUCCESS) {
		return dmscanlib::jni::createDecodeResultObject(env,result, dmScanLib.getDecodedWells());
	}
	return dmscanlib::jni::createDecodeResultObject(env, result);
}

