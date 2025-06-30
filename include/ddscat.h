#ifndef DDSCAT
#define DDSCAT

#include <stddef.h>
#include "mymath.h"

#define MAX_LINE_LENGTH 1024
#define MAX_NCOMP 100
#define MAX_NPLANES 100
#define MAX_COMP_LENGTH 512
#define PLANE_PARAMS 4

typedef enum {
    DDSCAT_OK,
    DDSCAT_ERR_CANNOT_OPEN_FILE,
    DDSCAT_ERR_PARSING_FAILED,
    DDSCAT_ERR_ALLOCATION_FAILED

} DdscatError;

typedef struct {
    size_t ncomp;
    size_t nplanes;
    char **comp;
    double (*planes)[PLANE_PARAMS];
    ComplexVec3 e01;
} Par;

typedef struct {
    size_t n;
    Complex *f11, *f21, *f12, *f22;
} Fmat;


typedef struct {
    size_t n;
    Complex *S1, *S2, *S3, *S4;
} Smat; 

DdscatError ddscat_parse_par_file(const char *par_file_path, Par *par);
DdscatError ddscat_parse_fml_file(const char *fml_file_path, const Par *par, Fmat **fmat);
DdscatError ddscat_calculate_scatmat(const Par *par, const Fmat *fmat, Smat **smat);

#endif  // DDSCAT
