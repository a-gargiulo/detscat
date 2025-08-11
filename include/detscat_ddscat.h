#ifndef DETSCAT_DDSCAT_H
#define DETSCAT_DDSCAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "mymath.h"

#define DETSCAT_DDSCAT_LINE_MAX 1024
#define DETSCAT_DDSCAT_PARSER_ERR_MSG_MAX 256
#define DETSCAT_DDSCAT_COMPONENTS_MAX 512
#define DETSCAT_DDSCAT_SCAT_PLANE_PARAMS 4

typedef struct {
    int w;  // Wavelength
    int r;  // Target size
    int k;  // Orientation
} DetScatDdscatCaseId;

typedef enum {

    DETSCAT_DDSCAT_PARSER_OK = 0,
    DETSCAT_DDSCAT_PARSER_ERR_ALLOC,
    DETSCAT_DDSCAT_PARSER_ERR_FORMAT

} DetScatDdscatParserStatus;

typedef struct {
    FILE *file;

    int line_number;

    DetScatDdscatParserStatus status;

    bool eof;

    char line[DETSCAT_DDSCAT_LINE_MAX];
    char err_msg[DETSCAT_DDSCAT_PARSER_ERR_MSG_MAX];

} DetScatDdscatParser;

typedef struct {
    ComplexVec3 e01;

    size_t n_components;
    size_t n_scat_planes;

    char **components;

    double (*scat_planes)[DETSCAT_DDSCAT_SCAT_PLANE_PARAMS];

} DetScatDdscatParams;

typedef struct {
    Complex *f11, *f21, *f12, *f22;

    size_t n_theta;

    double *theta;
    double phi;

} DetScatDdscatFmatrix;


DetScatDdscatParser *detscat_ddscat_parser_create(const char *ddscat_file_path);

void detscat_ddscat_parser_free(DetScatDdscatParser *ddscat_parser);

bool detscat_ddscat_parser_parse_par(DetScatDdscatParser *ddscat_parser, DetScatDdscatParams *par);

bool detscat_ddscat_parser_parse_fml(const char *fml_file_path,
                                     const DetScatDdscatParams *par,
                                     DetScatDdscatFmatrix **fmat);

void detscat_ddscat_par_free(DetScatDdscatParams *par);

void detscat_ddscat_fmat_free(DetScatDdscatFmatrix *fmat);

#endif  // DETSCAT_DDSCAT_H
