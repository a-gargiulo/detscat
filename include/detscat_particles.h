#ifndef DETSCAT_PARTICLES_H
#define DETSCAT_PARTICLES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "detscat_ddscat_util.h"
#include "mymath.h"

#define DETSCAT_PARTICLES_LINE_MAX 1024
#define DETSCAT_PARTICLES_TYPE_ID_MAX 128 
#define DETSCAT_PARTICLES_DATA_DIR_MAX 512
#define DETSCAT_PARTICLES_PARSER_ERR_MSG_MAX 256

typedef struct {

    Vec3 position;

    DdscatCaseId case_id;

    char *type_id;

} DetScatParticleDef;

typedef struct {

    char *type_id;
    char *data_dir;

} DetScatParticleTypeDef;

typedef struct {

    size_t n_types;
    size_t n_particles;

    DetScatParticleTypeDef *types;
    DetScatParticleDef *particles;

} DetScatParticlesData;

typedef enum {

    DETSCAT_PARTICLES_PARSER_OK = 0,
    DETSCAT_PARTICLES_PARSER_ERR_FORMAT,
    DETSCAT_PARTICLES_PARSER_ERR_ALLOC,
    DETSCAT_PARTICLES_PARSER_ERR_INVALID_INPUT

} DetScatParticlesParserStatus;

typedef struct {
    FILE *file;

    int line_number;

    DetScatParticlesParserStatus status;

    bool eof;

    char line[DETSCAT_PARTICLES_LINE_MAX];
    char err_msg[DETSCAT_PARTICLES_PARSER_ERR_MSG_MAX];

} DetScatParticlesParser;

DetScatParticlesParser *detscat_particles_parser_create(
    const char *particles_def_file_path);

bool detscat_particles_parser_parse(DetScatParticlesParser *pr_parser,
                                    DetScatParticlesData *pr_data);

void detscat_particles_parser_free(DetScatParticlesParser *pr_parser);

void detscat_particles_data_free(DetScatParticlesData *pr_data);

#endif  // DETSCAT_PARTICLES_H
