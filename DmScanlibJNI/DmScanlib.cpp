#include <jni.h>
#include <stdio.h>
#include "DmScanlib.h"

using namespace std;

//////////////////////////////////Scan Settings/////////////////////////////////////
class ScanSettings {

public:
	ScanSettings(JNIEnv *env, jobject scanSettings);
	int getDpi();
	int getBrightness();
	int getContrast();

private:
	int dpi;
	int brightness;
	int contrast;

};

ScanSettings::ScanSettings(JNIEnv *env, jobject scanSettings) {

	jclass scanSettings_Class = env->GetObjectClass(scanSettings);

	jfieldID scanSettingsClass_Field_Dpi = env->GetFieldID(scanSettings_Class,
			"dpi", "I");

	jfieldID scanSettingsClass_Field_Brightness = env->GetFieldID(
			scanSettings_Class, "brightness", "I");

	jfieldID scanSettingsClass_Field_Contrast = env->GetFieldID(
			scanSettings_Class, "contrast", "I");

	dpi = env->GetIntField(scanSettings, scanSettingsClass_Field_Dpi);
	brightness = env->GetIntField(scanSettings,
			scanSettingsClass_Field_Brightness);
	contrast = env->GetIntField(scanSettings, scanSettingsClass_Field_Contrast);
}

int ScanSettings::getDpi() {
	return dpi;
}
int ScanSettings::getBrightness() {
	return brightness;
}
int ScanSettings::getContrast() {
	return contrast;
}
////////////////////////////////// ScanCoordinates /////////////////////////////////////

class ScanCoordinates {

public:
	ScanCoordinates(JNIEnv *env, jobject scanCoordinates);
	double getLeft();
	double getTop();
	double getRight();
	double getBottom();

private:
	double left, top, right, bottom;

};

ScanCoordinates::ScanCoordinates(JNIEnv *env, jobject scanCoordinates) {

	jclass scanCoordinates_Class = env->GetObjectClass(scanCoordinates);

	jfieldID scanSettingsClass_Field_Left = env->GetFieldID(
			scanCoordinates_Class, "left", "D");

	jfieldID scanSettingsClass_Field_Top = env->GetFieldID(
			scanCoordinates_Class, "top", "D");

	jfieldID scanSettingsClass_Field_Right = env->GetFieldID(
			scanCoordinates_Class, "right", "D");

	jfieldID scanSettingsClass_Field_Bottom = env->GetFieldID(
			scanCoordinates_Class, "bottom", "D");

	left = env->GetDoubleField(scanCoordinates, scanSettingsClass_Field_Left);
	top = env->GetDoubleField(scanCoordinates, scanSettingsClass_Field_Top);
	right = env->GetDoubleField(scanCoordinates, scanSettingsClass_Field_Right);
	bottom = env->GetDoubleField(scanCoordinates,
			scanSettingsClass_Field_Bottom);

}

double ScanCoordinates::getLeft() {
	return left;
}
double ScanCoordinates::getTop() {
	return top;
}
double ScanCoordinates::getRight() {
	return right;
}
double ScanCoordinates::getBottom() {
	return bottom;
}
////////////////////////////////// DecodeProfile /////////////////////////////////////
class DecodeProfile {

public:
	DecodeProfile(JNIEnv *env, jobject decodeProfile);
	int getProfileA();
	int getProfileB();
	int getProfileC();

private:
	int profileA;
	int profileB;
	int profileC;

};

DecodeProfile::DecodeProfile(JNIEnv *env, jobject decodeProfile) {
	jclass decodeProfile_Class = env->GetObjectClass(decodeProfile);

	jfieldID scanSettingsClass_Field_ProfileA = env->GetFieldID(
			decodeProfile_Class, "profileA", "I");
	jfieldID scanSettingsClass_Field_ProfileB = env->GetFieldID(
			decodeProfile_Class, "profileB", "I");
	jfieldID scanSettingsClass_Field_ProfileC = env->GetFieldID(
			decodeProfile_Class, "profileC", "I");

	profileA = env->GetIntField(decodeProfile,
			scanSettingsClass_Field_ProfileA);
	profileB = env->GetIntField(decodeProfile,
			scanSettingsClass_Field_ProfileB);
	profileC = env->GetIntField(decodeProfile,
			scanSettingsClass_Field_ProfileC);

}

int DecodeProfile::getProfileA() {
	return profileA;
}
int DecodeProfile::getProfileB() {
	return profileB;
}
int DecodeProfile::getProfileC() {
	return profileC;
}

////////////////////////////////// DecodeSettings /////////////////////////////////////
class DecodeSettings {

public:
	DecodeSettings(JNIEnv *env, jobject decodeSettings);

	int getPlateNum();

	int getCorrections();
	int getSquareDev();
	int getEdgeThresh();

	double getScanGap();
	double getCellDistance();
	double getGapX();
	double getGapY();

	bool isVertical();

private:
	int plateNum;

	int corrections;
	int squareDev;
	int edgeThresh;

	double scanGap;
	double cellDistance;
	double gapX;
	double gapY;

	bool vertical;
};

DecodeSettings::DecodeSettings(JNIEnv *env, jobject decodeSettings) {
	jclass decodeSettings_Class = env->GetObjectClass(decodeSettings);

	jfieldID decodeSettings_Class_Field_plateNum = env->GetFieldID(
			decodeSettings_Class, "plateNum", "I");
	jfieldID decodeSettings_Class_Field_corrections = env->GetFieldID(
			decodeSettings_Class, "corrections", "I");
	jfieldID decodeSettings_Class_Field_squareDev = env->GetFieldID(
			decodeSettings_Class, "squareDev", "I");
	jfieldID decodeSettings_Class_Field_edgeThresh = env->GetFieldID(
			decodeSettings_Class, "edgeThresh", "I");

	plateNum = env->GetIntField(decodeSettings,
			decodeSettings_Class_Field_plateNum);
	corrections = env->GetIntField(decodeSettings,
			decodeSettings_Class_Field_corrections);
	squareDev = env->GetIntField(decodeSettings,
			decodeSettings_Class_Field_squareDev);
	edgeThresh = env->GetIntField(decodeSettings,
			decodeSettings_Class_Field_edgeThresh);

	jfieldID decodeSettings_Class_Field_scanGap = env->GetFieldID(
			decodeSettings_Class, "scanGap", "D");
	jfieldID decodeSettings_Class_Field_cellDistance = env->GetFieldID(
			decodeSettings_Class, "cellDistance", "D");
	jfieldID decodeSettings_Class_Field_gapX = env->GetFieldID(
			decodeSettings_Class, "gapX", "D");
	jfieldID decodeSettings_Class_Field_gapY = env->GetFieldID(
			decodeSettings_Class, "gapY", "D");

	scanGap = env->GetDoubleField(decodeSettings,
			decodeSettings_Class_Field_scanGap);
	cellDistance = env->GetDoubleField(decodeSettings,
			decodeSettings_Class_Field_cellDistance);
	gapX = env->GetDoubleField(decodeSettings, decodeSettings_Class_Field_gapX);
	gapY = env->GetDoubleField(decodeSettings, decodeSettings_Class_Field_gapY);

	jfieldID decodeSettings_Class_Field_isVertical = env->GetFieldID(
			decodeSettings_Class, "isVertical", "Z");
	vertical = env->GetBooleanField(decodeSettings,
			decodeSettings_Class_Field_isVertical);

}

int DecodeSettings::getPlateNum() {
	return plateNum;
}
int DecodeSettings::getCorrections() {
	return corrections;
}
int DecodeSettings::getSquareDev() {
	return squareDev;
}
int DecodeSettings::getEdgeThresh() {
	return edgeThresh;
}

double DecodeSettings::getScanGap() {
	return scanGap;
}
double DecodeSettings::getCellDistance() {
	return cellDistance;
}
double DecodeSettings::getGapX() {
	return gapX;
}
double DecodeSettings::getGapY() {
	return gapY;
}

bool DecodeSettings::isVertical() {
	return vertical;
}
////////////////////////////////// generateBarcodes /////////////////////////////////////

jobjectArray generateJStringArray(JNIEnv *env, jclass obj, char *message[],
		int messageLength) {

	jobjectArray stringsArray;

	stringsArray = (jobjectArray) env->NewObjectArray(messageLength,
			env->FindClass("java/lang/String"), env->NewStringUTF(""));

	for (int i = 0; i < messageLength; i++) {
		env->SetObjectArrayElement(stringsArray, i,
				env->NewStringUTF(message[i]));
	}

	return stringsArray;
}
////////////////////////////////// createReturnCodeEnumObject /////////////////////////////////////
jobject createEnumObject(JNIEnv * env,char * enumName,char * enumValue){

	jclass getEnumMethodClass = env->FindClass("DmScanlib");
	jmethodID getEnumMethod = env->GetStaticMethodID(getEnumMethodClass, "getEnum",
			"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;");

	jvalue args[2];
	args[0].l = env->NewStringUTF(enumName);
	args[1].l = env->NewStringUTF(enumValue);

	// run "javap -s -p ScanlibData" to obtain method signatures from a class.
	jobject enumObject = env->CallObjectMethodA(getEnumMethodClass,getEnumMethod,args);
	return enumObject;
}

////////////////////////////////// createScanlibDataObject /////////////////////////////////////

jobject createScanlibDataObject(JNIEnv * env, jobjectArray barcodes,
		char * returnMessage, int returnCode, char * returnEnumValue) {

	jclass scanlibDataClass = env->FindClass("ScanlibData");
	jmethodID initScanlibMethod = env->GetMethodID(scanlibDataClass, "<init>",
			"([Ljava/lang/String;Ljava/lang/String;ILDmScanlibReturnCode;)V");
	// run "javap -s -p ScanlibData" to obtain method signatures from a class.

	jvalue data[4];
	data[0].l = barcodes;
	data[1].l = env->NewStringUTF(returnMessage);
	data[2].i = returnCode;
	data[3].l =createEnumObject(env,(char *)"DmScanlibReturnCode",returnEnumValue);

	jobject scanLibDataClass = env->NewObjectA(scanlibDataClass,
			initScanlibMethod, data);

	return scanLibDataClass;
}

JNIEXPORT jobject JNICALL Java_DmScanlib_getScanlibDataTest(JNIEnv *env,
		jclass obj, jint _verbose, jstring _filename, jobject _scanSettings,
		jobject _scanCoordinates, jobject _decodeProfile,
		jobject _decodeSettings) {

	int verbose = _verbose;
	const char *filename = env->GetStringUTFChars(_filename, 0);

	ScanSettings scanSettings(env, _scanSettings);
	ScanCoordinates scanCoordinates(env, _scanCoordinates);
	DecodeProfile decodeProfile(env, _decodeProfile);
	DecodeSettings decodeSettings(env, _decodeSettings);

	//char *message[2]; message[0] = (char *) "bat";
	char *message[5] = { (char *) "first", (char *) "second", (char *) "third",
			(char *) "fourth", (char *) "fifth" };
	jobjectArray barcodes = generateJStringArray(env, obj, message, 5);

	int scanSettingsSumInt = scanSettings.getDpi()
			+ scanSettings.getBrightness() + scanSettings.getContrast();

	int decodeProfileSumInt = decodeProfile.getProfileA()
			+ decodeProfile.getProfileB() + decodeProfile.getProfileC();

	int decodeSettingsInt = decodeSettings.getPlateNum()
			+ decodeSettings.getCorrections() + decodeSettings.getSquareDev()
			+ decodeSettings.getEdgeThresh();

	int vertical = decodeSettings.isVertical() ? 1 : 0;

	double scanCoordinateSumDbl = scanCoordinates.getTop()
			+ scanCoordinates.getLeft() + scanCoordinates.getRight()
			+ scanCoordinates.getBottom();

	double decodeSettingsDbl = decodeSettings.getScanGap()
			+ decodeSettings.getCellDistance() + decodeSettings.getGapX()
			+ decodeSettings.getGapY();

	int returnValue = (scanSettingsSumInt + decodeProfileSumInt
			+ decodeSettingsInt) * (verbose + vertical)
			+ (int) ((scanCoordinateSumDbl + decodeSettingsDbl) * 10000) << 10;

	jobject returnObject = createScanlibDataObject(env, barcodes,
			(char *) filename, returnValue,(char *)"SC_SUCCESS");

	env->ReleaseStringUTFChars(_filename, filename); //DO NOT FORGET THIS

	return returnObject;
}

JNIEXPORT jobject JNICALL Java_DmScanlib_slIsTwainAvailable(JNIEnv *env,
		jclass obj) {

	int twainAvailable = 5;

	jobject returnObject = createScanlibDataObject(env, NULL,
			(char *) "success", twainAvailable,(char *)"SC_SUCCESS");

	return returnObject;
}

JNIEXPORT jobject JNICALL Java_DmScanlib_slSelectSourceAsDefault(JNIEnv *env,
		jclass obj) {

	int selectSourceAsDefault = 6;

	jobject returnObject = createScanlibDataObject(env, NULL,
			(char *) "scanner 1", selectSourceAsDefault,(char *)"SC_SUCCESS");

	return returnObject;
}

JNIEXPORT jobject JNICALL Java_DmScanlib_slGetScannerCapability(JNIEnv *env,
		jclass obj) {

	int scannerCapability = 7;

	jobject returnObject = createScanlibDataObject(env, NULL, (char *) "very capable", scannerCapability,(char *)"SC_SUCCESS");

	return returnObject;
}

JNIEXPORT jobject JNICALL Java_DmScanlib_slScanFlatbed(JNIEnv *env, jclass obj,
		jint _verbose, jstring _filename, jobject _scanSettings) {

	int verbose = _verbose;
	const char *filename = env->GetStringUTFChars(_filename, 0);

	ScanSettings scanSettings(env, _scanSettings);

	jobject returnObject = createScanlibDataObject(
			env,
			NULL,
			(char *) filename,
			scanSettings.getDpi() + scanSettings.getBrightness()
					+ scanSettings.getContrast() + verbose,(char *)"SC_SUCCESS");

	env->ReleaseStringUTFChars(_filename, filename);

	return returnObject;
}

JNIEXPORT jobject JNICALL Java_DmScanlib_slScanImage(JNIEnv *env, jclass obj,
		jint _verbose, jstring _filename, jobject _scanSettings,
		jobject _scanCoordinates) {

	int verbose = _verbose;
	const char *filename = env->GetStringUTFChars(_filename, 0);

	ScanSettings scanSettings(env, _scanSettings);
	ScanCoordinates scanCoordinates(env, _scanCoordinates);

	jobject returnObject = createScanlibDataObject(
			env,
			NULL,
			(char *) filename,
			scanSettings.getDpi() + scanSettings.getBrightness()
					+ scanSettings.getContrast() + scanCoordinates.getBottom()
					+ scanCoordinates.getLeft() + scanCoordinates.getRight()
					+ scanCoordinates.getTop() + verbose,(char *)"SC_SUCCESS");

	env->ReleaseStringUTFChars(_filename, filename);

	return returnObject;
}

JNIEXPORT jobject JNICALL Java_DmScanlib_slDecodePlate(JNIEnv *env, jclass obj,
		jint _verbose, jobject _scanSettings, jobject _scanCoordinates,
		jobject _decodeProfile, jobject _decodeSettings) {

	int verbose = _verbose;

	ScanSettings scanSettings(env, _scanSettings);
	ScanCoordinates scanCoordinates(env, _scanCoordinates);
	DecodeProfile decodeProfile(env, _decodeProfile);
	DecodeSettings decodeSettings(env, _decodeSettings);

	int decodeProfileSumInt = decodeProfile.getProfileA()
			+ decodeProfile.getProfileB() + decodeProfile.getProfileC();

	int decodeSettingsInt = decodeSettings.getPlateNum()
			+ decodeSettings.getCorrections() + decodeSettings.getSquareDev()
			+ decodeSettings.getEdgeThresh();

	int vertical = decodeSettings.isVertical() ? 1 : 0;

	double decodeSettingsDbl = decodeSettings.getScanGap()
			+ decodeSettings.getCellDistance() + decodeSettings.getGapX()
			+ decodeSettings.getGapY();

	jobject returnObject = createScanlibDataObject(
			env,
			NULL,
			(char *) "henry jones",
			scanSettings.getDpi() + scanSettings.getBrightness()
					+ scanSettings.getContrast() + scanCoordinates.getBottom()
					+ scanCoordinates.getLeft() + scanCoordinates.getRight()
					+ scanCoordinates.getTop() + decodeProfileSumInt
					+ decodeSettingsDbl + decodeSettingsInt + vertical
					+ verbose,(char *)"SC_SUCCESS");

	return returnObject;
}

JNIEXPORT jobject JNICALL Java_DmScanlib_slDecodeImage(JNIEnv *env, jclass obj,
		jint _verbose, jstring _filename, jobject _decodeProfile,
		jobject _decodeSettings) {


	int verbose = _verbose;
	const char *filename = env->GetStringUTFChars(_filename, 0);

	DecodeProfile decodeProfile(env, _decodeProfile);
	DecodeSettings decodeSettings(env, _decodeSettings);

	int decodeProfileSumInt = decodeProfile.getProfileA()
			+ decodeProfile.getProfileB() + decodeProfile.getProfileC();

	int decodeSettingsInt = decodeSettings.getPlateNum()
			+ decodeSettings.getCorrections() + decodeSettings.getSquareDev()
			+ decodeSettings.getEdgeThresh();

	int vertical = decodeSettings.isVertical() ? 1 : 0;

	double decodeSettingsDbl = decodeSettings.getScanGap()
			+ decodeSettings.getCellDistance() + decodeSettings.getGapX()
			+ decodeSettings.getGapY();

	jobject returnObject = createScanlibDataObject(
			env,
			NULL,
			(char *) filename,
			decodeProfileSumInt + decodeSettingsDbl + decodeSettingsInt
					+ vertical + verbose,(char *)"SC_SUCCESS");

	env->ReleaseStringUTFChars(_filename, filename);

	return returnObject;
}

