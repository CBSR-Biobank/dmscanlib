#ifndef __INCLUDE_SCAN_REGION
#define  __INCLUDE_SCAN_REGION

#include <jni.h>

class ScanRegion {
public:
    ScanRegion(JNIEnv *env, jobject scanRegionObj);

    double left;
    double top;
    double right;
    double bottom;

private:
};


#endif /* __INCLUDE_SCAN_REGION */
