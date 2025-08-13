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
    DETSCAT_DDSCAT_PARSER_ERR_FORMAT,
    DETSCAT_DDSCAT_PARSER_ERR_RESET

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
    double phi;

    size_t n;

    Complex *f11, *f21, *f12, *f22;
    double *theta;

} DetScatDdscatFmatrix;

typedef struct {

    size_t n_fmats;

    DetScatDdscatFmatrix *fmats;

} DetScatDdscatFml;

typedef struct {
    size_t n_pars;
    size_t n_fmls;
    size_t n_par_idx;

    DetScatDdscatParams *pars;
    DetScatDdscatFml *fmls;

    size_t *par_idx;

} DetScatDdscatData;


DetScatDdscatParser *detscat_ddscat_parser_create(const char *ddscat_file_path);

void detscat_ddscat_parser_free(DetScatDdscatParser *ddscat_parser);

bool detscat_ddscat_parser_reset(DetScatDdscatParser *ddscat_parser, const char *ddscat_file_path);

bool detscat_ddscat_parser_parse_par(DetScatDdscatParser *ddscat_parser, DetScatDdscatParams *par);

bool detscat_ddscat_parser_parse_fml(DetScatDdscatParser *ddscat_parser,
                                     DetScatDdscatFml *fml,
                                     DetScatDdscatParams *par);

void detscat_ddscat_par_free(DetScatDdscatParams *par);

void detscat_ddscat_fml_free(DetScatDdscatFml *fml);

void detscat_ddscat_data_free(DetScatDdscatData *ddscat);

#endif  // DETSCAT_DDSCAT_H
