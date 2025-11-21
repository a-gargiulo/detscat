#ifndef DETSCAT_MATH_H
#define DETSCAT_MATH_H

#include <stdbool.h>
#include <stddef.h>


typedef struct {
    double re, im;
} Complex;

typedef struct {
    Complex x, y;
} ComplexVec2;

typedef struct {
    Complex x, y, z;
} ComplexVec3;

typedef struct {
    double x, y, z;
} Vec3;

typedef struct {
    Complex f11, f12;
    Complex f21, f22;
} ComplexMat2;

typedef struct {
    double m11, m12, m13;
    double m21, m22, m23;
    double m31, m32, m33;
} Mat3; // row-major, m11,m12,m13 is first row

typedef enum {
    AXIS_X,
    AXIS_Y,
    AXIS_Z,
    AXIS_CAMERA
} Axis;


// Vec3
double ds_math_v3norm(const Vec3 *v);
double ds_math_v3dot(const Vec3 *v1, const Vec3 *v2);
void ds_math_v3cross(Vec3 *out, const Vec3 *v1, const Vec3 *v2);
void ds_math_v3add(Vec3 *out, const Vec3 *v1, const Vec3 *v2);
void ds_math_v3sub(Vec3 *out, const Vec3 *v1, const Vec3 *v2);
void ds_math_v3mul_el(Vec3 *out, const Vec3* v1, const Vec3 *v2);
void ds_math_v3div_el(Vec3 *out, const Vec3* v1, const Vec3 *v2);
void ds_math_v3scale(Vec3 *out, const Vec3 *v, double s); 
void ds_math_v3normalize(Vec3 *out, const Vec3 *v);
void ds_math_v3centroid(Vec3 *out, const void *varr, size_t n, size_t stride,
                        size_t pos_offset);
void ds_math_v3perp_ref(Vec3 *out, const Vec3 *dir);
void ds_math_v3basis_from_dir(Vec3 *x_out, Vec3 *y_out, Vec3 *z_out, 
                              const Vec3 *dir, const Vec3 *ref_vec,
                              Axis primary_axis);

// ---
// TODO: to be refactored
void detscat_math_vec3_orientation_to_angles(const Vec3 *v1, const Vec3 *v2, double *theta, double *beta, double *phi);
void detscat_math_vec3_angles_to_orientation(double theta, double beta, double phi, Vec3 *v1, Vec3 *v2);
double detscat_math_vec3_diff(const Vec3 *v1, const Vec3 *v2);
// ---


// Complex
double ds_math_cabs(Complex c);
Complex ds_math_cadd(Complex c1, Complex c2);
Complex ds_math_csub(Complex c1, Complex c2);
Complex ds_math_cmul(Complex c1, Complex c2);
Complex ds_math_cmul_real(Complex c, double r);
Complex ds_math_conj(Complex c);
Complex ds_math_cexp(Complex c);


// ComplexVec3
double ds_math_cv3norm(const ComplexVec3 *c);
Complex ds_math_cv3dot(const ComplexVec3 *c1, const ComplexVec3 *c2);
void ds_math_cv3cross(ComplexVec3 *out, const ComplexVec3 *c1,
                      const ComplexVec3 *c2);
void ds_math_cv3add(ComplexVec3 *out, const ComplexVec3 *c1,
                    const ComplexVec3 *c2);



void detscat_math_cplx_vec3_conj(ComplexVec3 *conj, const ComplexVec3 *c);
void detscat_math_cplx_vec3_normalize(ComplexVec3 *e, const ComplexVec3 *c,
                                         double abs);


void detscat_math_cplx_vec3_add_real(ComplexVec3 *vsum, const ComplexVec3 *v1, const Vec3 *v2);

// ComplexVec2
double detscat_math_cplx_vec2_abs(const ComplexVec2 *c);
void detscat_math_cplx_vec2_normalize(ComplexVec2 *e, const ComplexVec2 *c, double abs);
void detscat_math_cplx_vec2_scale(ComplexVec2 *out, const ComplexVec2 *v, Complex c); 
void detscat_math_cplx_vec2_add(ComplexVec2 *vsum, const ComplexVec2 *v1, const ComplexVec2 *v2);

// ComplexMat2
void detscat_math_cplx_mat2_cplx_vec2_mult(ComplexVec2 *vout, const ComplexMat2 *m, const ComplexVec2 *v);




// Mat3
void detscat_math_mat3_transpose(Mat3 *transpose, const Mat3 *m);
void detscat_math_mat3_vec3_mult(Vec3 *vout, const Mat3 *m, const Vec3 *v);
void detscat_math_mat3_cplx_vec3_mult(ComplexVec3 *vout, const Mat3 *m, const ComplexVec3 *v);
void detscat_math_mat3_mat3_mult(Mat3 *mout, const Mat3 *m1, const Mat3 *m2);
void detscat_math_mat3_basis_to_rotmat(Mat3 *mout, const Vec3 *x, const Vec3 *y, const Vec3 *z);

// Utilities
void detscat_math_bracket_value(const void *arr, size_t n, size_t stride, size_t var_offset, double target,
                                size_t *idx_low, size_t *idx_high);

double detscat_math_linterp(double x, double f0, double f1, double x0, double x1);

Complex detscat_math_cplx_linterp(double x, Complex f0, Complex f1, double x0, double x1);

#endif  // DETSCAT_MATH_H
