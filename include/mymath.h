#ifndef MYMATH_H
#define MYMATH_H

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
double mymath_complex_vec3_abs(const ComplexVec3 *c);                                               // Euclidean norm of a complex 3d vector

Complex mymath_complex_vec3_dot(const ComplexVec3 *c1, const ComplexVec3 *c2);                      // Dot product between two complex 3d vectors

void mymath_complex_vec3_cross(ComplexVec3 *cross, const ComplexVec3 *c1, const ComplexVec3 *c2);   // Cross product between two complex 3d vectors

void mymath_complex_vec3_conj(ComplexVec3 *conj, const ComplexVec3 *c);                             // Complex conjugate for a complex 3d vector

void mymath_complex_vec3_normalize(ComplexVec3 *e, const ComplexVec3 *c, double abs);               // Nomalizes a COMPLEX 3d vector

// Vec3
double mymath_vec3_abs(const Vec3 *v);                                                              // Euclidean norm of a 3d vector

void mymath_vec3_cross(Vec3 *cross, const Vec3 *v1, const Vec3 *v2);                                // Cross product between two vectors

// Complex
double mymath_complex_abs(Complex c);                                                               // Absolute value of a complex number

Complex mymath_complex_mult(Complex c1, Complex c2);                                                // Product of two complex numbers

Complex mymath_complex_add(Complex c1, Complex c2);                                                 // Sum of two complex numbers

Complex mymath_complex_sub(Complex c1, Complex c2);                                                 // Difference of two complex numbers

Complex mymath_complex_conj(Complex c);                                                             // Complex conjugate of a complex number

#endif  // MYMATH_H
