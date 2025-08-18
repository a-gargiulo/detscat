#ifndef DETSCAT_PARTICLES_H
#define DETSCAT_PARTICLES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "detscat_ddscat.h"

#include "mymath.h"

#define DETSCAT_PRT_LINE_MAX 1024
#define DETSCAT_PRT_TYPEID_MAX 128 
#define DETSCAT_PRT_DATADIR_MAX 512
#define DETSCAT_PRT_ERRMSG_MAX 256

typedef struct {
    Vec3 position;
    DetScatDdscatCaseId caseid;
    char *typeid;
} DetScatPrtDef;

typedef struct {
    char *typeid;
    char *datadir;
} DetScatPrtTypeDef;

typedef struct {
    size_t n_types;
    size_t n_particles;
    DetScatPrtTypeDef *types;
    DetScatPrtDef *particles;
} DetScatPrtData;

typedef enum {
    DETSCAT_PRT_PARSER_OK = 0,
    DETSCAT_PRT_PARSER_ERR_FORMAT,
    DETSCAT_PRT_PARSER_ERR_ALLOC,
    DETSCAT_PRT_PARSER_ERR_INPUT
} DetScatPrtParserStatus;

typedef struct {
    FILE *file;
    int line_number;
    DetScatPrtParserStatus status;
    bool eof;
    char line[DETSCAT_PRT_LINE_MAX];
    char errmsg[DETSCAT_PRT_ERRMSG_MAX];
} DetScatPrtParser;

bool detscat_prt_parser_init(DetScatPrtParser *parser, const char *file_path);
bool detscat_prt_parser_load(DetScatPrtParser *parser, DetScatPrtData *prt);
void detscat_prt_parser_close(DetScatPrtParser *parser);
void detscat_prt_free(DetScatPrtData *prt);

#endif  // DETSCAT_PARTICLES_H
