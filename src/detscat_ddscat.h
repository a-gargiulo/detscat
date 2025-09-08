#ifndef DETSCAT_DDSCAT_H
#define DETSCAT_DDSCAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "detscat.h"
#include "detscat_math.h"
#include "detscat_str.h"

#define DETSCAT_DDSCAT_LINE_MAX 1024

/**
 * wxxxryyykzzz.fml - 16 bytes
 */
#define DETSCAT_DDSCAT_FML_FILENAME_LEN 16

/**
 * ddscat.par - 10 bytes
 */
#define DETSCAT_DDSCAT_PAR_FILENAME_LEN 10

struct DetScatPrt;


typedef double Plane[4];

typedef struct {
    ComplexVec3 e01;       // Incident light polarization basis vector
    Str *components;       // Component names
    Plane *scat_planes;    // Scattering plane parameters
    size_t n_components;   // Number of components forming the target
    size_t n_scat_planes;  // Number of scattering planes
} DetScatDdscatParams;

typedef struct {
    double phi;      // Azimuthal angle
    double *theta;   // Scattering angles
    Complex *f11;    // Scattering matrix component f11
    Complex *f21;    // Scattering matrix component f21
    Complex *f12;    // Scattering matrix component f12
    Complex *f22;    // Scattering matrix component f22
    size_t n_theta;  // Number of scattering angles, theta
} DetScatDdscatFmatrix;

typedef struct {
    DetScatDdscatFmatrix *fmats;
    size_t n_fmats;
} DetScatDdscatFml;

typedef struct {
    size_t n_pars;
    size_t n_fmls;
    size_t n_par_idxs;
    DetScatDdscatParams *pars;
    DetScatDdscatFml *fmls;
    size_t *par_idxs;
} DetScatDdscat;

bool detscat_ddscat_init(DetScatDdscat *ddscat, size_t n_pars,
                         size_t n_fmls, size_t n_par_idxs);

bool detscat_ddscat_load(DetScatDdscat *ddscat, struct DetScatPrt *prt, DetScatError *err);


bool detscat_ddscat_par_load(const char *file_path, DetScatDdscatParams *par,
                             DetScatError *err);

bool detscat_ddscat_fml_load(const char *file_path, DetScatDdscatFml *fml,
                             DetScatDdscatParams *par, DetScatError *err);

void detscat_ddscat_free(DetScatDdscat *ddscat);
void detscat_ddscat_par_free(DetScatDdscatParams *par);
void detscat_ddscat_fml_free(DetScatDdscatFml *fml);

#endif  // DETSCAT_DDSCAT_H
