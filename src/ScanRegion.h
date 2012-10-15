#ifndef __INCLUDE_SCAN_REGION
#define  __INCLUDE_SCAN_REGION

#include "geometry.h"

#include <jni.h>

/**
 * Defines the bounding box for the image to be retrieved from the flatbed
 * scanner.
 */
class ScanRegion {
public:
    ScanRegion(JNIEnv *env, jobject scanRegionObj);
    virtual ~ScanRegion() { };

    BoundingBox<double> boundingBox;

private:
};


#endif /* __INCLUDE_SCAN_REGION */
