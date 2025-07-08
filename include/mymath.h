#ifndef MYMATH
#define MYMATH

#define MJ2J 1e-03
#define NS2S 1e-09
#define MM2M 1e-03

typedef struct
{
    double re, im;
} Complex;

typedef struct
{
    Complex x, y, z;
} ComplexVec3;

typedef struct
{
    double x, y, z;
} Vec3;

// Complex vector operations
void mymath_cross(Vec3 *cross, const Vec3 *v1, const Vec3 *v2);
void mymath_cross_complex(ComplexVec3 *cross, const ComplexVec3 *c1, const ComplexVec3 *c2);
void mymath_vec_conj_complex(ComplexVec3 *conj, const ComplexVec3 *cvec);
void mymath_norm_vec_complex(ComplexVec3 *nvec, const ComplexVec3 *cvec, const double norm);
double mymath_norm_complex(const ComplexVec3 *cvec);
Complex mymath_cdot(const ComplexVec3 *c1, const ComplexVec3 *c2);

// Complex value operations
double mymath_cabs(Complex c);
Complex mymath_cmult(Complex c1, Complex c2);
Complex mymath_cadd(Complex c1, Complex c2);
Complex mymath_csub(Complex c1, Complex c2);
Complex mymath_conj(Complex c);

#endif // MYMATH
