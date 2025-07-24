#ifndef DETSCAT_PARTICLES_H
#define DETSCAT_PARTICLES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "mymath.h"

#define DETSCAT_PARTICLES_LINE_MAX 1024
#define DETSCAT_PARTICLES_PARSER_ERR_MSG_MAX 256

typedef struct {
    char *id;
    int w;
    int r;
    int k;
    Vec3 position;
} DetScatParticle;

typedef struct {
    char *id;
    char *data_dir;
} DetScatParticleDefinition;

typedef struct {
    size_t n_definitions;                    // Number of particle definitions
    size_t n_particles;                      // Number of particles
    DetScatParticleDefinition *definitions;  // Array of particle definitions
    DetScatParticle *particles;              // Array of partilces
} DetScatParticlesData;

typedef enum {
    DETSCAT_PARTICLES_PARSER_OK = 0,
    DETSCAT_PARTICLES_PARSER_ERR_FILE_NOT_FOUND,
    DETSCAT_PARTICLES_PARSER_ERR_ALLOC,
    DETSCAT_PARTICLES_PARSER_ERR_INVALID_ARG,
    DETSCAT_PARTICLES_PARSER_ERR_FORMAT
} DetScatParticlesParserStatus;

typedef struct {
    FILE *file;
    int line_number;
    DetScatParticlesParserStatus status;
    bool eof;
    char line[DETSCAT_PARTICLES_LINE_MAX];
    char err_msg[DETSCAT_PARTICLES_PARSER_ERR_MSG_MAX];
} DetScatParticlesParser;

// Open and initialize parser from file path; returns parser or NULL on failure.
DetScatParticlesParser *detscat_particles_parser_create(const char *file_path);

// Parse file into particles data struct; returns true on success, false on error.
bool detscat_particles_parser_parse(DetScatParticlesParser *parser, DetScatParticlesData *data);

// Close file and free parser memory.
void detscat_particles_parser_free(DetScatParticlesParser *parser);

// Free particles data struct memory.
void detscat_particles_data_free(DetScatParticlesData *data);

#endif  // DETSCAT_PARTICLES_H
