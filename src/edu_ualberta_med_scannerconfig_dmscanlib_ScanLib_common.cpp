/*
 * Contains JNI code common to both MS Windows and Linux.
 */

#include "edu_ualberta_med_scannerconfig_dmscanlib_ScanLib.h"
#include "DmScanLib.h"
#include "ScanRegion.h"
#include "DmScanLibInternal.h"
#include "PalletCell.h"

#include <iostream>

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

jobject createDecodeResultObject(
                JNIEnv * env, int resultCode,
                std::vector<std::tr1::shared_ptr<PalletCell> > * cells) {
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

    jmethodID setCellMethod = env->GetMethodID(resultClass, "setCell",
                                               "(IILjava/lang/String;)V");

    if (cells != NULL) {
        for (unsigned i = 0, n = cells->size(); i < n; ++i) {
            PalletCell & cell = *(*cells)[i];
            jvalue data[3];

            data[0].i = cell.getRow();
            data[1].i = cell.getCol();
            data[2].l = env->NewStringUTF(cell.getBarcodeMsg().c_str());

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
                JNIEnv * env, jobject obj, jlong _verbose, jlong _plateNum,
                jstring _filename, jdouble scanGap, jlong _squareDev,
                jlong _edgeThresh, jlong _corrections, jdouble cellDistance,
                jdouble gapX, jdouble gapY, jlong _profileA, jlong _profileB,
                jlong _profileC, jlong _orientation) {

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
    int result = dmScanLib.decodeImage(plateNum, filename, scanGap,
                                       squareDev, edgeThresh, corrections,
                                       cellDistance, gapX, gapY, profileA,
                                       profileB, profileC, orientation);

    std::vector<std::tr1::shared_ptr<PalletCell> > * cells = NULL;
    if (result == SC_SUCCESS) {
        cells = &dmScanLib.getDecodedCells();
    }
    jobject resultObj = createDecodeResultObject(env, result, cells);
    env->ReleaseStringUTFChars(_filename, filename);
    return resultObj;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLib
 * Method:    decodeImage
 * Signature: (JLjava/lang/String;Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeOptions;[Ledu/ualberta/med/scannerconfig/dmscanlib/Well;)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLib_decodeImage
  (JNIEnv * env, jobject obj, jlong _verbose, jstring _filename, jobject decodeOptions, jobjectArray wells) {

    unsigned verbose = static_cast<unsigned>(_verbose);
    const char *filename = env->GetStringUTFChars(_filename, 0);

    DmScanLib dmScanLib(verbose);
}

