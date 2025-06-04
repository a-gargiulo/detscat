#ifndef MYMATH
#define MYMATH

double mymath_trapz(const double* f, const double* x);

#ifdef MYMATH_USE_NO_PREFIX
#define trapz mymath_trapz
#endif

#endif // MYMATH
