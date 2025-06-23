#ifndef MYMATH 
#define MYMATH 

typedef struct {
    double re, im;
} Complex;

typedef struct {
    Complex x, y, z;
} ComplexVec3;

double mymath_abs_complex(Complex *cnum);
double mymath_norm_complex(ComplexVec3 *cvec);

#endif  // MYMATH 
