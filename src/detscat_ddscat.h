#ifndef DETSCAT_DDSCAT_H
#define DETSCAT_DDSCAT_H

#include <stdbool.h>
#include <stddef.h>

#include "detscat.h"
#include "detscat_math.h"
#include "detscat_str.h"

struct DetScatPrt;

typedef double Plane[4];

typedef struct {
    ComplexVec3 e01;
    Str *components;
    Plane *scat_planes;
    size_t n_components;
    size_t n_scat_planes;
} DetScatDdscatParams;

typedef struct {
    double phi;
    double *theta;
    Complex *f11;
    Complex *f21;
    Complex *f12;
    Complex *f22;
    size_t n_theta;
} DetScatDdscatFmatrix;

typedef struct {
    size_t refcount;  // for safe freeing after caching
    DetScatDdscatFmatrix *fmats;
    size_t n_fmats;
} DetScatDdscatFml;

typedef struct {
    size_t n_pars;
    size_t n_fmls;
    size_t n_par_idxs;
    DetScatDdscatParams *pars;
    DetScatDdscatFml **fmls;  // "array of pointers to fml structs", double pointer due to caching
    size_t *par_idxs;
} DetScatDdscat;

bool detscat_ddscat_init(DetScatDdscat *ddscat, size_t n_pars, size_t n_fmls,
                         size_t n_par_idxs, DetScatError *err);
void detscat_ddscat_destroy(DetScatDdscat *ddscat);
void detscat_ddscat_par_free(DetScatDdscatParams *par);
void detscat_ddscat_par_free_subset(DetScatDdscatParams *par, size_t count);
void detscat_ddscat_fml_free(DetScatDdscatFml *fml);
void detscat_ddscat_fml_free_subset(DetScatDdscatFml *fml, size_t count);
bool detscat_ddscat_par_load(const char *par_file_path,
                             DetScatDdscatParams *par, DetScatError *err);
bool detscat_ddscat_fml_load(const char *fml_file_path, DetScatDdscatFml *fml,
                             DetScatDdscatParams *par, DetScatError *err);

#endif  // DETSCAT_DDSCAT_H
