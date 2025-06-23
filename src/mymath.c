#include "mymath.h"

#include <math.h>


double mymath_abs_complex(Complex *cnum) {
    return hypot(cnum->re, cnum->im);
}

double mymath_norm_complex(ComplexVec3 *cvec) {
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
