#include "ddscat.h"

#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mymath.h"

DdscatError ddscat_parse_par_file(const char *par_file_path, Par *par) {
    FILE *par_file = fopen(par_file_path, "r");
    if (!par_file) return DDSCAT_ERR_CANNOT_OPEN_FILE;

    char line[MAX_LINE_LENGTH];
    DdscatError status = DDSCAT_OK;

    size_t comp_allocated = 0;
    size_t comp_index = 0;
    size_t planes_parsed = 0;

    par->comp = NULL;
    par->planes = NULL;
    par->ncomp = 0;
    par->nplanes = 0;

    enum ParseState { PARSE_INITIAL, PARSE_COMP, PARSE_PLANES };
    enum ParseState state = PARSE_INITIAL;

    while (fgets(line, MAX_LINE_LENGTH, par_file)) {
        switch (state) {
            case PARSE_INITIAL:
                if (strstr(line, "NCOMP")) {
                    size_t ncomp;
                    if (sscanf(line, "%zu", &ncomp) != 1 || ncomp == 0 ||
                        ncomp >= MAX_NCOMP) {
                        status = DDSCAT_ERR_PARSING_FAILED;
                        goto cleanup;
                    }

                    par->ncomp = ncomp;
                    par->comp = (char **)malloc(ncomp * sizeof(char *));
                    if (!par->comp) {
                        status = DDSCAT_ERR_ALLOCATION_FAILED;
                        goto cleanup;
                    }

                    comp_allocated = 0;
                    comp_index = 0;
                    state = PARSE_COMP;
                } else if (strstr(line, "NPLANES")) {
                    size_t nplanes;
                    if (sscanf(line, "%zu", &nplanes) != 1 || nplanes == 0 ||
                        nplanes >= MAX_NPLANES) {
                        status = DDSCAT_ERR_PARSING_FAILED;
                        goto cleanup;
                    }

                    par->nplanes = nplanes;
                    par->planes = (double(*)[PLANE_PARAMS])malloc(
                        nplanes * sizeof(double[PLANE_PARAMS]));
                    if (!par->planes) {
                        status = DDSCAT_ERR_ALLOCATION_FAILED;
                        goto cleanup;
                    }

                    planes_parsed = 0;
                    state = PARSE_PLANES;
                } else if (strstr(line, "Polarization state")) {
                    if (sscanf(line, "(%lf, %lf) (%lf, %lf) (%lf, %lf)",
                               &par->e01.x.re, &par->e01.x.im, &par->e01.y.re,
                               &par->e01.y.im, &par->e01.z.re,
                               &par->e01.z.im) != 6) {
                        status = DDSCAT_ERR_PARSING_FAILED;
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
                    (char *)malloc(MAX_COMP_LENGTH * sizeof(char));
                if (!par->comp[comp_index]) {
                    status = DDSCAT_ERR_ALLOCATION_FAILED;
                    goto cleanup;
                }
                comp_allocated++;

                if (sscanf(line, "'%[^']'", par->comp[comp_index]) != 1) {
                    status = DDSCAT_ERR_PARSING_FAILED;
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
                           &par->planes[planes_parsed][3]) != PLANE_PARAMS) {
                    status = DDSCAT_ERR_PARSING_FAILED;
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

    if (status != DDSCAT_OK) {
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

DdscatError ddscat_parse_fml_file(const char *fml_file_path, const Par *par,
                                  Fmat **fmat) {
    FILE *fml_file = fopen(fml_file_path, "r");
    if (!fml_file) return DDSCAT_ERR_CANNOT_OPEN_FILE;

    char line[MAX_LINE_LENGTH];
    DdscatError status = DDSCAT_OK;

    size_t fmat_allocated = 0;

    while (fgets(line, MAX_LINE_LENGTH, fml_file)) {
        if (strstr(line, "Re")) {
            break;
        }
    }

    *fmat = (Fmat *)malloc(par->nplanes * sizeof(Fmat));
    if (!*fmat) {
        status = DDSCAT_ERR_ALLOCATION_FAILED;
        goto cleanup;
    }
    for (size_t i = 0; i < par->nplanes; ++i) {
        // TODO: Add some safety checks before casting
        size_t n_theta = (size_t)((par->planes[i][2] - par->planes[i][1]) /
                                  par->planes[i][3]) +
                         1;

        (*fmat)[i].n = n_theta;

        (*fmat)[i].f11 = (*fmat)[i].f12 = (*fmat)[i].f21 = (*fmat)[i].f22 =
            NULL;
        (*fmat)[i].f11 = (Complex *)malloc(n_theta * sizeof(Complex));
        (*fmat)[i].f12 = (Complex *)malloc(n_theta * sizeof(Complex));
        (*fmat)[i].f21 = (Complex *)malloc(n_theta * sizeof(Complex));
        (*fmat)[i].f22 = (Complex *)malloc(n_theta * sizeof(Complex));

        if (!(*fmat)[i].f11 || !(*fmat)[i].f12 || !(*fmat)[i].f21 ||
            !(*fmat)[i].f22) {
            status = DDSCAT_ERR_ALLOCATION_FAILED;
            goto cleanup;
        }

        fmat_allocated++;

        for (size_t j = 0; j < n_theta; ++j) {
            if (!fgets(line, MAX_LINE_LENGTH, fml_file)) {
                status = DDSCAT_ERR_PARSING_FAILED;
                goto cleanup;
            }
            if (sscanf(line, "%*lf %*lf %lf %lf %lf %lf %lf %lf %lf %lf",
                       &(*fmat)[i].f11[j].re, &(*fmat)[i].f11[j].im,
                       &(*fmat)[i].f21[j].re, &(*fmat)[i].f21[j].im,
                       &(*fmat)[i].f12[j].re, &(*fmat)[i].f12[j].im,
                       &(*fmat)[i].f22[j].re, &(*fmat)[i].f22[j].im) != 8) {
                status = DDSCAT_ERR_PARSING_FAILED;
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

DdscatError ddscat_calculate_scatmat(const Par *par, const Fmat *fmat,
                                     Smat **smat) {
    *smat = malloc(par->nplanes * sizeof(Smat));
    if (!(*smat)) {
        return DDSCAT_ERR_ALLOCATION_FAILED;
    }

    size_t scomp_allocated = 0;

    ComplexVec3 eHat01, eHat01conj;
    ComplexVec3 e02, eHat02;

    double e01norm = mymath_norm_complex(&par->e01);
    mymath_norm_vec_complex(&eHat01, &par->e01, e01norm);
    mymath_vec_conj_complex(&eHat01conj, &eHat01);

    mymath_cross_complex(
        &e02, &(const ComplexVec3){{1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0}},
        &eHat01conj);
    double e02norm = mymath_norm_complex(&e02);
    mymath_norm_vec_complex(&eHat02, &e02, e02norm);

    Complex a = mymath_cdot(
        &eHat01, &(const ComplexVec3){{0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}});

    Complex b = mymath_cdot(
        &eHat01, &(const ComplexVec3){{0.0, 0.0}, {0.0, 0.0}, {1.0, 0.0}});

    Complex c = mymath_cdot(
        &eHat02, &(const ComplexVec3){{0.0, 0.0}, {1.0, 0.0}, {0.0, 0.0}});

    Complex d = mymath_cdot(
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
            return DDSCAT_ERR_ALLOCATION_FAILED;
        }
        scomp_allocated++;

        for (size_t j = 0; j < fmat[i].n; ++j) {
            Complex cp = {cos(phi), 0.0};
            Complex sp = {sin(phi), 0.0};

            (*smat)[i].S1[j] = mymath_cmult(
                (Complex){0.0, -1.0},
                mymath_cadd(mymath_cmult(fmat[i].f21[j],
                                         mymath_csub(mymath_cmult(b, cp),
                                                     mymath_cmult(a, sp))),
                            mymath_cmult(fmat[i].f22[j],
                                         mymath_csub(mymath_cmult(d, cp),
                                                     mymath_cmult(c, sp)))));




            (*smat)[i].S2[j] = mymath_cmult(
                (Complex){0.0, -1.0},
                mymath_cadd(mymath_cmult(fmat[i].f11[j],
                                         mymath_cadd(mymath_cmult(a, cp),
                                                     mymath_cmult(b, sp))),
                            mymath_cmult(fmat[i].f12[j],
                                         mymath_cadd(mymath_cmult(c, cp),
                                                     mymath_cmult(d, sp)))));


            (*smat)[i].S3[j] = mymath_cmult(
                (Complex){0.0, 1.0},
                mymath_cadd(mymath_cmult(fmat[i].f11[j],
                                         mymath_csub(mymath_cmult(b, cp),
                                                     mymath_cmult(a, sp))),
                            mymath_cmult(fmat[i].f12[j],
                                         mymath_csub(mymath_cmult(d, cp),
                                                     mymath_cmult(c, sp)))));


            (*smat)[i].S4[j] = mymath_cmult(
                (Complex){0.0, 1.0},
                mymath_cadd(mymath_cmult(fmat[i].f21[j],
                                         mymath_cadd(mymath_cmult(a, cp),
                                                     mymath_cmult(b, sp))),
                            mymath_cmult(fmat[i].f22[j],
                                         mymath_cadd(mymath_cmult(c, cp),
                                                     mymath_cmult(d, sp)))));

            // DIAGNOSTICS! Remove later
            // double S43 = mymath_csub(
            //     mymath_cmult((*smat)[i].S1[j], mymath_conj((*smat)[i].S2[j])), 
            //     mymath_cmult((*smat)[i].S3[j], mymath_conj((*smat)[i].S4[j])) 
            // ).im;
            // printf("%22.15E\n", S43);
        }
    }

    return DDSCAT_OK;
}
