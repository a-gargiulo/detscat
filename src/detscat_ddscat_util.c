#include "detscat_ddscat_util.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mymath.h"

DetScatDdscatUtilStatus detscat_ddscat_util_parse_par_file(const char *par_file_path, DdscatPar *par) {
    assert(par != NULL);
    assert(par_file_path != NULL);

    FILE *par_file = fopen(par_file_path, "r");
    if (!par_file) return DETSCAT_DDSCAT_UTIL_ERR_CANNOT_OPEN_FILE;

    DetScatDdscatUtilStatus status = DETSCAT_DDSCAT_UTIL_OK;

    size_t comp_allocated = 0;
    size_t comp_index = 0;
    size_t planes_parsed = 0;

    par->comp = NULL;
    par->planes = NULL;
    par->ncomp = 0;
    par->nplanes = 0;

    enum DdscatParseState { PARSE_INITIAL, PARSE_COMP, PARSE_PLANES };
    enum DdscatParseState state = PARSE_INITIAL;

    char line[DETSCAT_DDSCAT_UTIL_LINE_MAX];

    while (fgets(line, DETSCAT_DDSCAT_UTIL_LINE_MAX, par_file)) {
        switch (state) {
            case PARSE_INITIAL:
                if (strstr(line, "NCOMP")) {
                    size_t ncomp;
                    if (sscanf(line, "%zu", &ncomp) != 1 || ncomp == 0 ||
                        ncomp >= DETSCAT_DDSCAT_UTIL_NCOMP_MAX) {
                        status = DETSCAT_DDSCAT_UTIL_ERR_PARSING;
                        goto cleanup;
                    }

                    par->ncomp = ncomp;
                    par->comp = (char **)malloc(ncomp * sizeof(char *));
                    if (!par->comp) {
                        status = DETSCAT_DDSCAT_UTIL_ERR_ALLOC;
                        goto cleanup;
                    }

                    comp_allocated = 0;
                    comp_index = 0;
                    state = PARSE_COMP;
                } else if (strstr(line, "NPLANES")) {
                    size_t nplanes;
                    if (sscanf(line, "%zu", &nplanes) != 1 || nplanes == 0 ||
                        nplanes >= DETSCAT_DDSCAT_UTIL_NPLANES_MAX) {
                        status = DETSCAT_DDSCAT_UTIL_ERR_PARSING;
                        goto cleanup;
                    }

                    par->nplanes = nplanes;
                    par->planes = (double(*)[DETSCAT_DDSCAT_UTIL_PLANE_PARAMS])malloc(
                        nplanes * sizeof(double[DETSCAT_DDSCAT_UTIL_PLANE_PARAMS]));
                    if (!par->planes) {
                        status = DETSCAT_DDSCAT_UTIL_ERR_ALLOC;
                        goto cleanup;
                    }

                    planes_parsed = 0;
                    state = PARSE_PLANES;
                } else if (strstr(line, "Polarization state")) {
                    if (sscanf(line, "(%lf, %lf) (%lf, %lf) (%lf, %lf)",
                               &par->e01.x.re, &par->e01.x.im, &par->e01.y.re,
                               &par->e01.y.im, &par->e01.z.re,
                               &par->e01.z.im) != 6) {
                        status = DETSCAT_DDSCAT_UTIL_ERR_PARSING;
                        goto cleanup;
                    }
                }
                break;

            case PARSE_COMP:
                if (comp_index >= par->ncomp) {
                    state = PARSE_INITIAL;
                    break;
                }

                par->comp[comp_index] =
                    (char *)malloc(DETSCAT_DDSCAT_UTIL_COMP_MAX * sizeof(char));
                if (!par->comp[comp_index]) {
                    status = DETSCAT_DDSCAT_UTIL_ERR_ALLOC;
                    goto cleanup;
                }
                comp_allocated++;

                if (sscanf(line, "'%[^']'", par->comp[comp_index]) != 1) {
                    status = DETSCAT_DDSCAT_UTIL_ERR_PARSING;
                    goto cleanup;
                }

                comp_index++;
                if (comp_index == par->ncomp) {
                    state = PARSE_INITIAL;
                }
                break;

            case PARSE_PLANES:
                if (planes_parsed >= par->nplanes) {
                    state = PARSE_INITIAL;
                    break;
                }

                if (sscanf(line, "%lf %lf %lf %lf",
                           &par->planes[planes_parsed][0],
                           &par->planes[planes_parsed][1],
                           &par->planes[planes_parsed][2],
                           &par->planes[planes_parsed][3]) != DETSCAT_DDSCAT_UTIL_PLANE_PARAMS) {
                    status = DETSCAT_DDSCAT_UTIL_ERR_PARSING;
                    goto cleanup;
                }

                planes_parsed++;
                if (planes_parsed == par->nplanes) {
                    state = PARSE_INITIAL;
                }
                break;
        }
    }
    goto cleanup;

cleanup:
    fclose(par_file);

    if (status != DETSCAT_DDSCAT_UTIL_OK) {
        if (par->comp) {
            for (size_t i = 0; i < comp_allocated; ++i) {
                free(par->comp[i]);
            }
            free(par->comp);
            par->comp = NULL;
        }

        if (par->planes) {
            free(par->planes);
            par->planes = NULL;
        }

        par->ncomp = 0;
        par->nplanes = 0;
    }

    return status;
}

DetScatDdscatUtilStatus detscat_ddscat_util_parse_fml_file(const char *fml_file_path, const DdscatPar *par,
                                  Fmat **fmat) {
    FILE *fml_file = fopen(fml_file_path, "r");
    if (!fml_file) return DETSCAT_DDSCAT_UTIL_ERR_CANNOT_OPEN_FILE;

    char line[DETSCAT_DDSCAT_UTIL_LINE_MAX];
    DetScatDdscatUtilStatus status = DETSCAT_DDSCAT_UTIL_OK;

    size_t fmat_allocated = 0;

    while (fgets(line, DETSCAT_DDSCAT_UTIL_LINE_MAX, fml_file)) {
        if (strstr(line, "Re")) {
            break;
        }
    }

    *fmat = (Fmat *)malloc(par->nplanes * sizeof(Fmat));
    if (!*fmat) {
        status = DETSCAT_DDSCAT_UTIL_ERR_ALLOC;
        goto cleanup;
    }
    for (size_t i = 0; i < par->nplanes; ++i) {
        // TODO: Add some safety checks before casting
        size_t n_theta = (size_t)((par->planes[i][2] - par->planes[i][1]) /
                                  par->planes[i][3]) +
                         1;

        (*fmat)[i].phi = par->planes[i][0];
        (*fmat)[i].n = n_theta;

        (*fmat)[i].theta = NULL;

        (*fmat)[i].f11 = (*fmat)[i].f12 = (*fmat)[i].f21 = (*fmat)[i].f22 =
            NULL;
        (*fmat)[i].theta = malloc(n_theta * sizeof(double));
        (*fmat)[i].f11 = malloc(n_theta * sizeof(Complex));
        (*fmat)[i].f12 = malloc(n_theta * sizeof(Complex));
        (*fmat)[i].f21 = malloc(n_theta * sizeof(Complex));
        (*fmat)[i].f22 = malloc(n_theta * sizeof(Complex));

        if (!(*fmat)[i].theta || !(*fmat)[i].f11 || !(*fmat)[i].f12 || !(*fmat)[i].f21 ||
            !(*fmat)[i].f22) {
            status = DETSCAT_DDSCAT_UTIL_ERR_ALLOC;
            goto cleanup;
        }

        fmat_allocated++;

        for (size_t j = 0; j < n_theta; ++j) {
            if (!fgets(line, DETSCAT_DDSCAT_UTIL_LINE_MAX, fml_file)) {
                status = DETSCAT_DDSCAT_UTIL_ERR_PARSING;
                goto cleanup;
            }
            if (sscanf(line, "%lf %*f %lf %lf %lf %lf %lf %lf %lf %lf",
                       &(*fmat)[i].theta[j],
                       &(*fmat)[i].f11[j].re, &(*fmat)[i].f11[j].im,
                       &(*fmat)[i].f21[j].re, &(*fmat)[i].f21[j].im,
                       &(*fmat)[i].f12[j].re, &(*fmat)[i].f12[j].im,
                       &(*fmat)[i].f22[j].re, &(*fmat)[i].f22[j].im) != 9) {
                status = DETSCAT_DDSCAT_UTIL_ERR_PARSING;
                goto cleanup;
            }
        }
    }
    fclose(fml_file);
    return status;

cleanup:
    fclose(fml_file);

    if (*fmat) {
        for (size_t i = 0; i < fmat_allocated; ++i) {
            free((*fmat)[i].theta);
            free((*fmat)[i].f11);
            free((*fmat)[i].f21);
            free((*fmat)[i].f12);
            free((*fmat)[i].f22);
        }
        free(*fmat);
        *fmat = NULL;
    }

    return status;
}

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
