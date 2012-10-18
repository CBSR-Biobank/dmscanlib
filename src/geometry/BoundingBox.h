#ifndef __INCLUDE_SCAN_REGION
#define  __INCLUDE_SCAN_REGION

#include "geometry.h"

#include <jni.h>

/**
 * Defines the bounding box for the image to be retrieved from the flatbed
 * scanner.
 */
class BoundingBox {
public:
    BoundingBox(JNIEnv *env, jobject scanRegionObj);
    virtual ~BoundingBox() { };

    const vector<const Point<T>> points;

private:
};


#endif /* __INCLUDE_SCAN_REGION */
