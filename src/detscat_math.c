#include "detscat_math.h"

#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

double detscat_math_cplx_vec3_abs(const ComplexVec3 *c) {
    assert(c);

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
    assert(c1 && c2);

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
    assert(cross && c1 && c2);

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
    assert(conj && c);

    conj->x.re = c->x.re;
    conj->x.im = -c->x.im;
    conj->y.re = c->y.re;
    conj->y.im = -c->y.im;
    conj->z.re = c->z.re;
    conj->z.im = -c->z.im;
}

void detscat_math_cplx_vec3_normalize(ComplexVec3 *e, const ComplexVec3 *c,
                                      double abs) {
    assert(e && c);
    assert(abs != 0.0);

    e->x.re = c->x.re / abs;
    e->x.im = c->x.im / abs;
    e->y.re = c->y.re / abs;
    e->y.im = c->y.im / abs;
    e->z.re = c->z.re / abs;
    e->z.im = c->z.im / abs;
}

double detscat_math_vec3_abs(const Vec3 *v) {
    assert(v);

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

double detscat_math_vec3_dot(const Vec3 *v1, const Vec3 *v2) {
    assert(v1 && v2);

    return v1->x*v2->x + v1->y*v2->y + v1->z*v2->z;
}

void detscat_math_vec3_cross(Vec3 *cross, const Vec3 *v1, const Vec3 *v2) {
    assert(cross && v1 && v2);

    cross->x = v1->y * v2->z - v2->y * v1->z;
    cross->y = v1->z * v2->x - v1->x * v2->z;
    cross->z = v1->x * v2->y - v1->y * v2->x;
}

void detscat_math_vec3_add(Vec3 *vsum, const Vec3 *v1, const Vec3 *v2) {
    assert(vsum && v1 && v2);

    vsum->x = v1->x + v2->x;
    vsum->y = v1->y + v2->y;
    vsum->z = v1->z + v2->z;
}

void detscat_math_vec3_sub(Vec3 *vdiff, const Vec3 *v1, const Vec3 *v2) {
    assert(vdiff && v1 && v2);

    vdiff->x = v1->x - v2->x;
    vdiff->y = v1->y - v2->y;
    vdiff->z = v1->z - v2->z;
}

void detscat_math_vec3_normalize(Vec3 *e, const Vec3 *v, double abs) {
    assert(e && v);
    assert(abs != 0.0);

    e->x = v->x / abs;
    e->y = v->y / abs;
    e->z = v->z / abs;
}

void detscat_math_vec3_scale(Vec3 *out, const Vec3 *v, double s) {
    assert(out && v);
    out->x = v->x * s;
    out->y = v->y * s;
    out->z = v->z * s;
}

void detscat_math_vec3_centroid(Vec3 *out, const void *arr, size_t n, size_t stride, size_t pos_offset) {
    assert(out && arr);
    assert(n > 0);

    *out = (Vec3){0};

    const unsigned char *ptr = (const unsigned char*)arr;
    for (size_t i = 0; i < n; ++i) {
        const Vec3 *pos = (const Vec3 *)(ptr + pos_offset);
        out->x += pos->x;
        out->y += pos->y;
        out->z += pos->z;
        ptr += stride;
    }

    double inv_n = 1.0 / (double)n;
    out->x *= inv_n;
    out->y *= inv_n;
    out->z *= inv_n;
}

void detscat_math_vec3_perp_ref(Vec3 *out, const Vec3 *dir) {
    assert(out && dir);

    Vec3 dir_norm;
    detscat_math_vec3_normalize(&dir_norm, dir, detscat_math_vec3_abs(dir));

    Vec3 ref; 
    if (fabs(dir->x) <= fabs(dir->y) && fabs(dir->x) <= fabs(dir->z)) {
        ref = (Vec3){1.0, 0, 0};
    } else if (fabs(dir->y) <= fabs(dir->z)) {
        ref = (Vec3){0, 1.0, 0};
    } else {
        ref = (Vec3){0, 0, 1.0};
    }

    double dot = detscat_math_vec3_dot(&ref, &dir_norm);
    ref.x -= dot * dir_norm.x;
    ref.y -= dot * dir_norm.y;
    ref.z -= dot * dir_norm.z;

    double mag = detscat_math_vec3_abs(&ref);
    assert(mag > 1e-12);
    detscat_math_vec3_normalize(out, &ref, mag);
}

void detscat_math_vec3_build_basis(const Vec3 *dir, 
                                   const Vec3 *ref_vec,
                                   Axis primary_axis,
                                   Vec3 *x_out,
                                   Vec3 *y_out,
                                   Vec3 *z_out) {
    assert(dir && x_out && y_out && z_out);

    Vec3 primary;
    detscat_math_vec3_normalize(&primary, dir, detscat_math_vec3_abs(dir));

    Vec3 secondary;

    if (ref_vec) {
        secondary = *ref_vec;
        double dot = detscat_math_vec3_dot(&secondary, &primary);
        secondary.x -= dot * primary.x;
        secondary.y -= dot * primary.y;
        secondary.z -= dot * primary.z;

        double sec_mag = detscat_math_vec3_abs(&secondary);
        if (sec_mag < 1e-12) {
            detscat_math_vec3_perp_ref(&secondary, &primary);
        } else {
            detscat_math_vec3_normalize(&secondary, &secondary, sec_mag);
        }
    } else {
        detscat_math_vec3_perp_ref(&secondary, &primary);
    }

    Vec3 tertiary;
    detscat_math_vec3_cross(&tertiary, &primary, &secondary);
    double ter_mag = detscat_math_vec3_abs(&tertiary);
    assert(ter_mag > 1e-12);
    detscat_math_vec3_normalize(&tertiary, &tertiary, ter_mag);


    switch(primary_axis) {
        case AXIS_X:
            *x_out = primary;
            *y_out = secondary;
            *z_out = tertiary;
            break;
        case AXIS_Y:
            *x_out = tertiary;
            *y_out = primary;
            *z_out = secondary;
            break;
        case AXIS_Z:
            *x_out = secondary;
            *y_out = tertiary;
            *z_out = primary;
            break;
        case AXIS_CAMERA:
            *x_out = tertiary;
            detscat_math_vec3_scale(x_out, x_out, -1.0);
            *y_out = secondary;
            *z_out = primary;
            break;
        default:
            assert(0 && "Invalid primary_axis");
    }
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

void detscat_math_mat3_vec3_mult(Vec3 *vout, const Mat3 *m, const Vec3 *v) {
    assert(vout && m && v);

    double t1, t2, t3;

    t1 = m->m11 * v->x + m->m12 * v->y + m->m13 * v->z;
    t2 = m->m21 * v->x + m->m22 * v->y + m->m23 * v->z;
    t3 = m->m31 * v->x + m->m32 * v->y + m->m33 * v->z;

    *vout = (Vec3){t1, t2, t3};
}

void detscat_math_mat3_mat3_mult(Mat3 *mout, const Mat3 *m1, const Mat3 *m2) {
    assert(mout && m1 && m2);

    double t1, t2, t3, t4, t5, t6, t7, t8, t9;

    t1 = m1->m11 * m2->m11 + m1->m12 * m2->m21 + m1->m13 * m2->m31;
    t2 = m1->m11 * m2->m12 + m1->m12 * m2->m22 + m1->m13 * m2->m32;
    t3 = m1->m11 * m2->m13 + m1->m12 * m2->m23 + m1->m13 * m2->m33;
    t4 = m1->m21 * m2->m11 + m1->m22 * m2->m21 + m1->m23 * m2->m31;
    t5 = m1->m21 * m2->m12 + m1->m22 * m2->m22 + m1->m23 * m2->m32;
    t6 = m1->m21 * m2->m13 + m1->m22 * m2->m23 + m1->m23 * m2->m33;
    t7 = m1->m31 * m2->m11 + m1->m32 * m2->m21 + m1->m33 * m2->m31;
    t8 = m1->m31 * m2->m12 + m1->m32 * m2->m22 + m1->m33 * m2->m32;
    t9 = m1->m31 * m2->m13 + m1->m32 * m2->m23 + m1->m33 * m2->m33;

    *mout = (Mat3){t1, t2, t3, t4, t5, t6, t7, t8, t9};
}


void detscat_math_mat3_basis_to_rotmat(Mat3 *mout, const Vec3 *x, const Vec3 *y, const Vec3 *z) {
    assert(mout && x && y && z);

    *mout = (Mat3){x->x, x->y, x->z, 
             y->x, y->y, y->z,
             z->x, z->y, z->z};
}

