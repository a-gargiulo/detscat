#ifndef DETSCAT_TRANSFORM_H
#define DETSCAT_TRANSFORM_H

#include <stddef.h>

#include "detscat_math.h"
#include "detscat_prt.h"

typedef struct {
    Vec3 translation;
    Mat3 rotation;
} DetScatTransform;

void detscat_prt_transform(DetScatPrtParticle *particles, size_t n,
                           const Vec3 *origin, const Mat3 *rotmat);

#endif  // DETSCAT_TRANSFORM_H
