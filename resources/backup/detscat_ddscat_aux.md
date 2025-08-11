# detscat_ddscat.h

```c
typedef struct {
    size_t n;
    Complex *S1, *S2, *S3, *S4;
} Smat; 

DetScatDdscatUtilStatus detscat_ddscat_util_calculate_scatmat(const DdscatPar *par, const Fmat *fmat, Smat **smat);

```

# detscat_ddscat.c

```c
DetScatDdscatUtilStatus detscat_ddscat_util_calculate_scatmat(const DdscatPar *par, const Fmat *fmat,
                                     Smat **smat) {
    *smat = malloc(par->nplanes * sizeof(Smat));
    if (!(*smat)) {
        return DETSCAT_DDSCAT_UTIL_ERR_ALLOC;
    }

    size_t scomp_allocated = 0;

    ComplexVec3 eHat01, eHat01conj;
    ComplexVec3 e02, eHat02;

    double e01norm = mymath_complex_vec3_abs(&par->e01);
    mymath_complex_vec3_normalize(&eHat01, &par->e01, e01norm);
    mymath_complex_vec3_conj(&eHat01conj, &eHat01);

    mymath_complex_vec3_cross(
        &e02, &(const ComplexVec3){{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
        &eHat01conj);
    double e02norm = mymath_complex_vec3_abs(&e02);
    mymath_complex_vec3_normalize(&eHat02, &e02, e02norm);

    Complex a = mymath_complex_vec3_dot(
        &eHat01, &(const ComplexVec3){{0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}});

    Complex b = mymath_complex_vec3_dot(
        &eHat01, &(const ComplexVec3){{0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}});

    Complex c = mymath_complex_vec3_dot(
        &eHat02, &(const ComplexVec3){{0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}});

    Complex d = mymath_complex_vec3_dot(
        &eHat02, &(const ComplexVec3){{0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}});

    for (size_t i = 0; i < par->nplanes; ++i) {
        double phi = par->planes[i][0] * M_PI / 180.0;

        (*smat)[i].n = fmat[i].n;

        (*smat)[i].S1 = malloc(fmat[i].n * sizeof(Complex));
        (*smat)[i].S2 = malloc(fmat[i].n * sizeof(Complex));
        (*smat)[i].S3 = malloc(fmat[i].n * sizeof(Complex));
        (*smat)[i].S4 = malloc(fmat[i].n * sizeof(Complex));

        if (!((*smat)[i].S1) || !((*smat)[i].S2) || !((*smat)[i].S3) ||
            !((*smat)[i].S4)) {
            for (size_t m = 0; m < scomp_allocated; ++m) {
                free((*smat)[m].S1);
                free((*smat)[m].S2);
                free((*smat)[m].S3);
                free((*smat)[m].S4);
            }
            free((*smat));
            (*smat) = NULL;
            return DETSCAT_DDSCAT_UTIL_ERR_ALLOC;
        }
        scomp_allocated++;

        for (size_t j = 0; j < fmat[i].n; ++j) {
            Complex cp = {cos(phi), 0.0};
            Complex sp = {sin(phi), 0.0};

            (*smat)[i].S1[j] = mymath_complex_mult(
                (Complex){0.0, -1.0},
                mymath_complex_add(mymath_complex_mult(fmat[i].f21[j],
                                         mymath_complex_sub(mymath_complex_mult(b, cp),
                                                     mymath_complex_mult(a, sp))),
                            mymath_complex_mult(fmat[i].f22[j],
                                         mymath_complex_sub(mymath_complex_mult(d, cp),
                                                     mymath_complex_mult(c, sp)))));




            (*smat)[i].S2[j] = mymath_complex_mult(
                (Complex){0.0, -1.0},
                mymath_complex_add(mymath_complex_mult(fmat[i].f11[j],
                                         mymath_complex_add(mymath_complex_mult(a, cp),
                                                     mymath_complex_mult(b, sp))),
                            mymath_complex_mult(fmat[i].f12[j],
                                         mymath_complex_add(mymath_complex_mult(c, cp),
                                                     mymath_complex_mult(d, sp)))));


            (*smat)[i].S3[j] = mymath_complex_mult(
                (Complex){0.0, 1.0},
                mymath_complex_add(mymath_complex_mult(fmat[i].f11[j],
                                         mymath_complex_sub(mymath_complex_mult(b, cp),
                                                     mymath_complex_mult(a, sp))),
                            mymath_complex_mult(fmat[i].f12[j],
                                         mymath_complex_sub(mymath_complex_mult(d, cp),
                                                     mymath_complex_mult(c, sp)))));


            (*smat)[i].S4[j] = mymath_complex_mult(
                (Complex){0.0, 1.0},
                mymath_complex_add(mymath_complex_mult(fmat[i].f21[j],
                                         mymath_complex_add(mymath_complex_mult(a, cp),
                                                     mymath_complex_mult(b, sp))),
                            mymath_complex_mult(fmat[i].f22[j],
                                         mymath_complex_add(mymath_complex_mult(c, cp),
                                                     mymath_complex_mult(d, sp)))));

            // DIAGNOSTICS! Remove later
            // double S43 = mymath_csub(
            //     mymath_cmult((*smat)[i].S1[j], mymath_conj((*smat)[i].S2[j])), 
            //     mymath_cmult((*smat)[i].S3[j], mymath_conj((*smat)[i].S4[j])) 
            // ).im;
            // printf("%22.15E\n", S43);
        }
    }

    return DETSCAT_DDSCAT_UTIL_OK;
}
```
