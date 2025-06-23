#ifndef DDSCAT
#define DDSCAT

#include <stddef.h>

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
} Par;

typedef struct {
    double *f11r, *f11i;
    double *f21r, *f21i;
    double *f12r, *f12i;
    double *f22r, *f22i;
} Fmat;

DdscatError ddscat_parse_par_file(const char *par_file_path, Par *par);

DdscatError ddscat_parse_fml_file(const char *fml_file_path, Fmat *fmat);

#endif  // DDSCAT
