#ifndef MYMATH 
#define MYMATH 

typedef struct {
    double re, im;
} Complex;

typedef struct {
    Complex x, y, z;
} ComplexVec3;

typedef struct {
    double x, y, z;
} Vec3;

void mymath_cross(Vec3 *cross, const Vec3 *v1, const Vec3 *v2);

double mymath_abs_complex(const Complex *cnum);
double mymath_norm_complex(const ComplexVec3 *cvec);
void mymath_cross_complex(ComplexVec3 *cross, const ComplexVec3 *c1, const ComplexVec3 *c2);
void mymath_vec_conj_complex(ComplexVec3 *conj, const ComplexVec3 *cvec);
void mymath_norm_vec_complex(ComplexVec3 *nvec, const ComplexVec3 *cvec, const double norm);
Complex mymath_dot_complex(const ComplexVec3 *c1, const ComplexVec3 *c2);

#endif  // MYMATH 
