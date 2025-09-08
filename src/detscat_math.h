#ifndef DETSCAT_MATH_H
#define DETSCAT_MATH_H

typedef struct {
    double re, im;
} Complex;

typedef struct {
    Complex x, y, z;
} ComplexVec3;

typedef struct {
    double x, y, z;
} Vec3;

// ComplexVec3
double detscat_math_cplx_vec3_abs(const ComplexVec3 *c);
Complex detscat_math_cplx_vec3_dot(const ComplexVec3 *c1,
                                      const ComplexVec3 *c2);
void detscat_math_cplx_vec3_cross(ComplexVec3 *cross, const ComplexVec3 *c1,
                                     const ComplexVec3 *c2);
void detscat_math_cplx_vec3_conj(ComplexVec3 *conj, const ComplexVec3 *c);
void detscat_math_cplx_vec3_normalize(ComplexVec3 *e, const ComplexVec3 *c,
                                         double abs);

// Vec3
double detscat_math_vec3_abs(const Vec3 *v);
void detscat_math_vec3_cross(Vec3 *cross, const Vec3 *v1, const Vec3 *v2);
void detscat_math_vec3_add(Vec3 *vsum, const Vec3 *v1, const Vec3 *v2);
void detscat_math_vec3_sub(Vec3 *vdiff, const Vec3 *v1, const Vec3 *v2);
void detscat_math_vec3_normalize(Vec3 *e, const Vec3 *v, double abs);

// Complex
double detscat_math_cplx_abs(Complex c);
Complex detscat_math_cplx_mult(Complex c1, Complex c2);
Complex detscat_math_cplx_add(Complex c1, Complex c2);
Complex detscat_math_cplx_sub(Complex c1, Complex c2);
Complex detscat_math_cplx_conj(Complex c);

#endif  // DETSCAT_MATH_H
