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

double mymath_abs_complex(const Complex *cnum);
double mymath_norm_complex(const ComplexVec3 *cvec);
void mymath_cross(const Vec3 *v1, const Vec3 *v2, Vec3 *cross);

#endif  // MYMATH 
