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

DdscatError ddscat_parse_par_file(const char *par_file_path, Par *par);

#endif  // DDSCAT
