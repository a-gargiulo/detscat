#include "detscat_math.h"

#include <assert.h>
#include <math.h>
#include <string.h>

double detscat_math_cplx_vec3_abs(const ComplexVec3 *c) {
    double scale = 0.0;  // largest absolute value encountered
    double ssq = 1.0;    // sum of squares

    double comps[6] = {fabs(c->x.re), fabs(c->x.im), fabs(c->y.re),
                       fabs(c->y.im), fabs(c->z.re), fabs(c->z.im)};

    for (int i = 0; i < 6; i++) {
        double cc = comps[i];
        if (cc != 0.0) {
            if (scale < cc) {
                double t = scale / cc;
                ssq = 1.0 + ssq * t * t;  // rescaling with respect to new scale
                scale = cc;
            } else {
                double t = cc / scale;
                ssq += t * t;
            }
        }
    }
    return scale * sqrt(ssq);
}

Complex detscat_math_cplx_vec3_dot(const ComplexVec3 *c1,
                                   const ComplexVec3 *c2) {
    // Computes c1 * c2 = conj(c1) * c2
    // NOTE: conj(conj(c1) * c2) = c1 * conj(c2)

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

void detscat_math_cplx_vec3_cross(ComplexVec3 *cross, const ComplexVec3 *c1,
                                  const ComplexVec3 *c2) {
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

void detscat_math_cplx_vec3_conj(ComplexVec3 *conj, const ComplexVec3 *c) {
    conj->x.re = c->x.re;
    conj->x.im = -c->x.im;
    conj->y.re = c->y.re;
    conj->y.im = -c->y.im;
    conj->z.re = c->z.re;
    conj->z.im = -c->z.im;
}

void detscat_math_cplx_vec3_normalize(ComplexVec3 *e, const ComplexVec3 *c,
                                      double abs) {
    e->x.re = c->x.re / abs;
    e->x.im = c->x.im / abs;
    e->y.re = c->y.re / abs;
    e->y.im = c->y.im / abs;
    e->z.re = c->z.re / abs;
    e->z.im = c->z.im / abs;
}

double detscat_math_vec3_abs(const Vec3 *v) {
    double scale = 0.0;  // largest absolute value encountered
    double ssq = 1.0;    // sum of squares

    double comps[3] = {fabs(v->x), fabs(v->y), fabs(v->z)};

    for (int i = 0; i < 3; i++) {
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

void detscat_math_vec3_cross(Vec3 *cross, const Vec3 *v1, const Vec3 *v2) {
    cross->x = v1->y * v2->z - v2->y * v1->z;
    cross->y = v1->z * v2->x - v1->x * v2->z;
    cross->z = v1->x * v2->y - v1->y * v2->x;
}

void detscat_math_vec3_add(Vec3 *vsum, const Vec3 *v1, const Vec3 *v2) {
    vsum->x = v1->x + v2->x;
    vsum->y = v1->y + v2->y;
    vsum->z = v1->z + v2->z;
}

void detscat_math_vec3_sub(Vec3 *vdiff, const Vec3 *v1, const Vec3 *v2) {
    vdiff->x = v1->x - v2->x;
    vdiff->y = v1->y - v2->y;
    vdiff->z = v1->z - v2->z;
}

void detscat_math_vec3_normalize(Vec3 *e, const Vec3 *v, double abs) {
    e->x = v->x / abs;
    e->y = v->y / abs;
    e->z = v->z / abs;
}

double detscat_math_cplx_abs(Complex c) {
    return hypot(c.re, c.im);
}

Complex detscat_math_cplx_mult(Complex c1, Complex c2) {
    Complex result;
    result.re = c1.re * c2.re - c1.im * c2.im;
    result.im = c1.re * c2.im + c1.im * c2.re;
    return result;
}

Complex detscat_math_cplx_add(Complex c1, Complex c2) {
    Complex result;
    result.re = c1.re + c2.re;
    result.im = c1.im + c2.im;
    return result;
}

Complex detscat_math_cplx_sub(Complex c1, Complex c2) {
    Complex result;
    result.re = c1.re - c2.re;
    result.im = c1.im - c2.im;
    return result;
}

Complex detscat_math_cplx_conj(Complex c) {
    Complex result;
    result.re = c.re;
    result.im = -c.im;
    return result;
}
