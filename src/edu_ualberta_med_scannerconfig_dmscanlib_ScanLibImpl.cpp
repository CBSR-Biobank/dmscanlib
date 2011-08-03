#include <edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl.h>
#include "DmScanLib.h"

#include <iostream>

using namespace std;

const char * getResultCodeMsg(int resultCode) {
   const char * message = NULL;

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
   return message;
}

jobject createScanResultObject(JNIEnv * env, int resultCode, int value) {
   jclass scanLibResultClass = env->FindClass("edu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult");

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

jobject createDecodeResultObject(JNIEnv * env, int resultCode, int value) {
   jclass scanLibResultClass = env->FindClass("edu/ualberta/med/scannerconfig/dmscanlib/DecodeResult");

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

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slIsTwainAvailable
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slIsTwainAvailable(
   JNIEnv * env, jobject obj) {
   int result = slIsTwainAvailable();
   return createScanResultObject(env, result, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slSelectSourceAsDefault
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slSelectSourceAsDefault(
   JNIEnv * env, jobject obj) {
   int result = slSelectSourceAsDefault();
   return createScanResultObject(env, result, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slGetScannerCapability
 * Signature: ()Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slGetScannerCapability(
   JNIEnv * env, jobject obj) {
   int result = slGetScannerCapability();
   return createScanResultObject(env, SC_SUCCESS, result);
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slScanImage
 * Signature: (JJIIDDDDLjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slScanImage(
   JNIEnv * env, jobject obj, jlong verbose, jlong dpi, jint brightness, jint contrast, jdouble left, jdouble top, jdouble right,
   jdouble bottom, jstring _filename) {
   const char *filename = env->GetStringUTFChars(_filename, 0);
   env->ReleaseStringUTFChars(_filename, filename);
   return NULL;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slScanFlatbed
 * Signature: (JJIILjava/lang/String;)Ledu/ualberta/med/scannerconfig/dmscanlib/ScanLibResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slScanFlatbed(
   JNIEnv * env, jobject obj, jlong verbose, jlong dpi, jint brightness, jint contrast, jstring _filename) {
   const char *filename = env->GetStringUTFChars(_filename, 0);
   env->ReleaseStringUTFChars(_filename, filename);
   return NULL;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slDecodePlate
 * Signature: (JJIIJDDDDDJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slDecodePlate(
   JNIEnv * env, jobject obj, jlong verbose, jlong dpi, jint brightness, jint contrast, jlong plateNum, jdouble left, jdouble top,
   jdouble right, jdouble bottom, jdouble scanGap, jlong squareDev, jlong edgeThresh, jlong corrections, jdouble cellDistance, jdouble gapX,
   jdouble gapY, jlong profileA, jlong profileB, jlong profileC, jlong isVertical) {
   return NULL;
}

/*
 * Class:     edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl
 * Method:    slDecodeImage
 * Signature: (JJLjava/lang/String;DJJJDDDJJJJ)Ledu/ualberta/med/scannerconfig/dmscanlib/DecodeResult;
 */
JNIEXPORT jobject JNICALL Java_edu_ualberta_med_scannerconfig_dmscanlib_ScanLibImpl_slDecodeImage(
   JNIEnv * env, jobject obj, jlong verbose, jlong plateNum, jstring _filename, jdouble scanGap, jlong squareDev, jlong edgetThres, jlong corrections,
   jdouble cellDistance, jdouble gapX, jdouble gapY, jlong profileA, jlong profileB, jlong profileC, jlong isVertical) {
   const char *filename = env->GetStringUTFChars(_filename, 0);
   env->ReleaseStringUTFChars(_filename, filename);
   return NULL;
}

