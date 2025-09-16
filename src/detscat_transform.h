#ifndef DETSCAT_TRANSFORM_H
#define DETSCAT_TRANSFORM_H

#include "detscat_math.h"

typedef struct {
    Vec3 translation;
    Mat3 rotation;
} DetScatTransform;

#endif  // DETSCAT_TRANSFORM_H
