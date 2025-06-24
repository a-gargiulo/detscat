#include "mymath.h"

#include <math.h>


double mymath_abs_complex(const Complex *cnum) {
    return hypot(cnum->re, cnum->im);
}

double mymath_norm_complex(const ComplexVec3 *cvec) {
    double scale = 0.0; // largest absolute value encountered
    double ssq = 1.0;  // sum of squares

    double comps[6] = {
        fabs(cvec->x.re), fabs(cvec->x.im),
        fabs(cvec->y.re), fabs(cvec->y.im),
        fabs(cvec->z.re), fabs(cvec->z.im)
    };

    for (int i = 0; i < 6; i++) {
        double c = comps[i];
        if (c != 0.0) {
            if (scale < c) {
                double t = scale / c;
                ssq = 1.0 + ssq * t * t;  // rescaling with respect to new scale
                scale = c;
            } else {
                double t = c / scale;
                ssq += t * t;
            }
        }
    }
    return scale * sqrt(ssq);
}


void mymath_cross(const Vec3 *v1, const Vec3 *v2, Vec3 *cross) {
    cross->x = v1->y * v2->z - v2->y * v1->z;
    cross->y = v1->z * v2->x - v1->x * v2->z;
    cross->z = v1->x * v2->y - v1->y * v2->x;
}
