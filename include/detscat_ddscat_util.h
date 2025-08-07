#ifndef DETSCAT_DDSCAT_UTIL_H
#define DETSCAT_DDSCAT_UTIL_H

#include <stddef.h>
#include "mymath.h"

#define DETSCAT_DDSCAT_UTIL_LINE_MAX 1024
#define DETSCAT_DDSCAT_UTIL_NCOMP_MAX 100
#define DETSCAT_DDSCAT_UTIL_NPLANES_MAX 100
#define DETSCAT_DDSCAT_UTIL_COMP_MAX 512
#define DETSCAT_DDSCAT_UTIL_PLANE_PARAMS 4

typedef struct {
    int w;  // Wavelength
    int r;  // Target size
    int k;  // Orientation
} DdscatCaseId;

typedef enum {
    DETSCAT_DDSCAT_UTIL_OK,
    DETSCAT_DDSCAT_UTIL_ERR_CANNOT_OPEN_FILE,
    DETSCAT_DDSCAT_UTIL_ERR_PARSING,
    DETSCAT_DDSCAT_UTIL_ERR_ALLOC
} DetScatDdscatUtilStatus;

typedef struct {
    size_t ncomp;
    size_t nplanes;
    char **comp;
    double (*planes)[DETSCAT_DDSCAT_UTIL_PLANE_PARAMS];
    ComplexVec3 e01;
} DdscatPar;

typedef struct {
    size_t n;
    Complex *f11, *f21, *f12, *f22;
    double *theta;
    double phi;
} Fmat;


typedef struct {
    size_t n;
    Complex *S1, *S2, *S3, *S4;
} Smat; 

DetScatDdscatUtilStatus detscat_ddscat_util_parse_par_file(const char *par_file_path, DdscatPar *par);
DetScatDdscatUtilStatus detscat_ddscat_util_parse_fml_file(const char *fml_file_path, const DdscatPar *par, Fmat **fmat);
DetScatDdscatUtilStatus detscat_ddscat_util_calculate_scatmat(const DdscatPar *par, const Fmat *fmat, Smat **smat);

#endif  // DETSCAT_DDSCAT_UTIL_H
