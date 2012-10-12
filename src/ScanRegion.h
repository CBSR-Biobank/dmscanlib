#ifndef __INCLUDE_SCAN_REGION
#define  __INCLUDE_SCAN_REGION

#include "geometry.h"

#include <jni.h>

class ScanRegion {
public:
    ScanRegion(JNIEnv *env, jobject scanRegionObj);
    virtual ~ScanRegion() { };

    Point<double> point1;
    Point<double> point2;

private:
};


#endif /* __INCLUDE_SCAN_REGION */
