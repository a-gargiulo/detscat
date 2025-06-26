#include "mymath.h"

#include <math.h>
#include <string.h>


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

void mymath_cross(Vec3 *cross, const Vec3 *v1, const Vec3 *v2) {
    cross->x = v1->y * v2->z - v2->y * v1->z;
    cross->y = v1->z * v2->x - v1->x * v2->z;
    cross->z = v1->x * v2->y - v1->y * v2->x;
}


void mymath_cross_complex(ComplexVec3 *cross, const ComplexVec3 *c1, const ComplexVec3 *c2) {
    cross->x.re = c1->y.re * c2->z.re - c1->z.re * c2->y.re -
                  c1->y.im * c2->z.im + c1->z.im * c2->y.im;
    cross->x.im = c1->y.re * c2->z.im - c1->z.re * c2->y.im +
                  c1->y.im * c2->z.re - c1->z.im * c2->y.re;

    cross->y.re = c1->z.re * c2->x.re - c1->x.re * c2->z.re +
                  c1->x.im * c2->z.im - c1->z.im * c2->x.im;
    cross->y.im = c1->z.im * c2->x.re - c1->x.re * c2->z.im -
                  c1->x.im * c2->z.re + c1->z.re * c2->x.im;

    cross->z.re = c1->x.re * c2->y.re - c1->y.re * c2->x.re -
                  c1->x.im * c2->y.im + c1->y.im * c2->x.im;
    cross->z.im = c1->x.re * c2->y.im - c1->y.re * c2->x.im +
                  c1->x.im * c2->y.re - c1->y.im * c2->x.re;
}


void mymath_vec_conj_complex(ComplexVec3 *conj, const ComplexVec3 *cvec) {
    memcpy(conj, cvec, sizeof(ComplexVec3));
    conj->x.im *= -1.0;
    conj->y.im *= -1.0;
    conj->z.im *= -1.0;
}


void mymath_norm_vec_complex(ComplexVec3 *nvec, const ComplexVec3 *cvec, const double norm)
{
    nvec->x.re = cvec->x.re / norm;
    nvec->x.im = cvec->x.im / norm;
    nvec->y.re = cvec->y.re / norm;
    nvec->y.im = cvec->y.im / norm;
    nvec->z.re = cvec->z.re / norm;
    nvec->z.im = cvec->z.im / norm;
}


Complex mymath_dot_complex(const ComplexVec3 *c1, const ComplexVec3 *c2) {

    Complex result = {0.0, 0.0};

    double c1_re[3] = {c1->x.re, c1->y.re, c1->z.re};
    double c1_im[3] = {c1->x.im, c1->y.im, c1->z.im};
    double c2_re[3] = {c2->x.re, c2->y.re, c2->z.re};
    double c2_im[3] = {c2->x.im, c2->y.im, c2->z.im};

    for (size_t k = 0; k < 3; ++k) {
        result.re += c1_re[k] * c2_re[k] + c1_im[k] * c2_im[k];
        result.im += c1_re[k] * c2_im[k] - c1_im[k] * c2_re[k];
    }

    return result;
}

