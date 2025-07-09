#ifndef DETSCAT_PARTICLES_H
#define DETSCAT_PARTICLES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "mymath.h"

#define MAX_LINE_LENGTH 1024
#define MAX_ERROR_MESSAGE_LENGTH 256

typedef struct {
    char *type;
    Vec3 position;
} Particle;

typedef struct {
    char *type;
    char *data_path;
} ParticleType;

typedef struct {
    size_t n_types;
    size_t n_particles;
    ParticleType *types;
    Particle *particles;
} Particles;

typedef enum {
    DETSCAT_PARTICLE_PARSER_OK = 0,
    DETSCAT_PARTICLE_PARSER_ERR_FILE_NOT_FOUND,
    DETSCAT_PARTICLE_PARSER_ERR_ALLOCATION_FAILED,
    DETSCAT_PARTICLE_PARSER_ERR_INVALID_ARGUMENT,
    DETSCAT_PARTICLE_PARSER_ERR_INVALID_FORMAT,
    DETSCAT_PARTICLE_PARSER_ERR_UNKNOWN_KEY,
    DETSCAT_PARTICLE_PARSER_ERR_INVALID_VALUE
} ParticleParserError;

typedef struct {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    int line_number;
    bool eof;
    ParticleParserError error_code;
    char error_message[MAX_ERROR_MESSAGE_LENGTH];
} ParticleParser;

ParticleParser *detscat_particles_parser_init(const char *particles_file_path);
bool detscat_particles_parser_parse(ParticleParser *parser,
                                    Particles *particles);
void detscat_particles_parser_free(ParticleParser *parser);
void detscat_particles_free(Particles *particles);

#endif  // DETSCAT_PARTICLES_H
