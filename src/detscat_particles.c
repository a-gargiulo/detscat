#include "detscat_particles.h"

#include <assert.h>
#include <stdlib.h>
#include <strings.h>

#include "strutil.h"

DetScatParticlesParser *detscat_particles_parser_create(const char *particles_def_file_path) {

    // No assertions. Let the function fail if particles_def_file_path is invalid.
    // `NULL` will be returned upon error. The error should be safely caught upstream.
    DetScatParticlesParser *pr_parser = malloc(sizeof(DetScatParticlesParser));
    if (!pr_parser) return NULL;

    pr_parser->file = fopen(particles_def_file_path, "r");
    if (!pr_parser->file) {
        free(pr_parser);
        return NULL;
    }

    pr_parser->line_number = 0;
    pr_parser->eof = false;
    pr_parser->status = DETSCAT_PARTICLES_PARSER_OK;
    pr_parser->line[0] = '\0';
    pr_parser->err_msg[0] = '\0';

    return pr_parser;
}

void detscat_particles_parser_free(DetScatParticlesParser *pr_parser) {
    if (!pr_parser) return;

    if (pr_parser->file) {
        fclose(pr_parser->file);
        pr_parser->file = NULL;
    }

    free(pr_parser);
    return;
}

static void free_types(DetScatParticlesData *pr_data, size_t count) {
    if (!pr_data || !pr_data->types) return;
    for (size_t i = 0; i < count; ++i) {
        free(pr_data->types[i].type_id);
        free(pr_data->types[i].data_dir);
    }
    free(pr_data->types);
    pr_data->types = NULL;
    return;
}

static void free_particles(DetScatParticlesData *pr_data, size_t count) {
    if (!pr_data || !pr_data->particles) return;
    for (size_t i = 0; i < count; ++i) {
        free(pr_data->particles[i].type_id);
    }
    free(pr_data->particles);
    pr_data->particles = NULL;
    return;
}



bool detscat_particles_parser_parse(DetScatParticlesParser *pr_parser, DetScatParticlesData *pr_data) {
    assert(pr_parser != NULL);
    assert(pr_data != NULL);

    typedef enum {

        STATE_INITIAL,
        STATE_WAIT_SECTION,
        STATE_PARSE_TYPES_META,
        STATE_PARSE_TYPE_DEF,
        STATE_PARSE_PARTICLES_META,
        STATE_PARSE_PARTICLES_DEF,
        STATE_DONE,
        STATE_ERROR

    } ParserState;

    ParserState state = STATE_INITIAL;
    size_t types_allocated = 0;
    size_t particles_allocated = 0;

    while (fgets(pr_parser->line, sizeof(pr_parser->line), pr_parser->file)) {
        pr_parser->line_number++;


    }


    
}

// bool detscat_particles_parser_parse(DetScatParticlesParser *pr_parser, DetScatParticlesData *pr_data) {
//     assert(pr_parser != NULL);
//     assert(pr_data != NULL);

//     bool types_def_section = false;
//     bool particles_def_section = false;

//     while (fgets(pr_parser->line, sizeof(pr_parser->line), pr_parser->file)) {
//         pr_parser->line_number++;

//         char *trimmed = strutil_trim(pr_parser->line);
//         if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

//         if (strcmp(trimmed, "$(StartDef)") == 0) {
//             types_def_section = true;

//             bool types_def_meta_parsed = false;
//             bool types_def_end_found = false;

//             size_t types_def_allocated = 0;

//             while (fgets(pr_parser->line, sizeof(pr_parser->line), pr_parser->file)) {
//                 pr_parser->line_number++;

//                 char *trimmed = strutil_trim(pr_parser->line);
//                 if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

//                 if (strcmp(trimmed, "$(EndDef)") == 0) {
//                     types_def_end_found = true;
//                     break;
//                 }

//                 if (!types_def_meta_parsed) {
//                     if (sscanf(trimmed, " %zu ", &pr_data->n_types) != 1) {
//                         pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//                         snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
//                                  "Missing number of particle type definitions on line %d",
//                                  pr_parser->line_number);
//                         return false;
//                     }

//                     if (pr_data->n_types == 0) {
//                         pr_parser->status =  DETSCAT_PARTICLES_PARSER_ERR_INVALID_INPUT;
//                         snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
//                                  "Number of particle type definitions cannot be zero.");
//                         return false;
//                     }

//                     pr_data->types = malloc(pr_data->n_types * sizeof(DetScatParticleTypeDef));
//                     if (!pr_data->types) {
//                         pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
//                         snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
//                                  "Could not allocate particle type definitions array");
//                         return false;
//                     }
//                     memset(pr_data->types, 0, pr_data->n_types * sizeof(DetScatParticleTypeDef));

//                     types_def_meta_parsed = true;
//                     continue;
//                 }

//                 char tmp_id[64];
//                 char tmp_data_dir[1024];

//                 if (sscanf(trimmed, " %63s %1023s ", tmp_id, tmp_data_dir) != 2) {
//                     parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//                     snprintf(parser->err_msg, sizeof(parser->err_msg),
//                              "Invalid particle definition on line %d", parser->line_number);
//                     free_types(data, definitions_allocated);
//                     return false;
//                 }

//                 if (definitions_allocated >= data->n_definitions) {
//                     parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//                     snprintf(parser->err_msg, sizeof(parser->err_msg),
//                              "Too many particle definitions. Stopped at line %d",
//                              parser->line_number);
//                     free_types(data, definitions_allocated);
//                     return false;
//                 }

//                 data->definitions[definitions_allocated].id = strdup(tmp_id);
//                 data->definitions[definitions_allocated].data_dir =
//                     strdup(strutil_normpath(tmp_data_dir));

//                 if (!data->definitions[definitions_allocated].id ||
//                     !data->definitions[definitions_allocated].data_dir) {
//                     parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
//                     snprintf(parser->err_msg, sizeof(parser->err_msg),
//                              "Could not allocate particle definition on line %d",
//                              parser->line_number);
//                     if (data->definitions[definitions_allocated].id)
//                         free(data->definitions[definitions_allocated].id);
//                     if (data->definitions[definitions_allocated].data_dir)
//                         free(data->definitions[definitions_allocated].data_dir);
//                     free_types(data, definitions_allocated);
//                     return false;
//                 }

//                 definitions_allocated++;
//             }

//             if (!end_def_found) {
//                 parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//                 snprintf(parser->err_msg, sizeof(parser->err_msg),
//                          "Could not find $(EndDef) until line %d", parser->line_number);
//                 free_types(data, definitions_allocated);
//                 return false;
//             }

//             if (definitions_allocated != data->n_definitions) {
//                 parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//                 snprintf(parser->err_msg, sizeof(parser->err_msg),
//                          "Expected %zu particle definitions, got %zu", data->n_definitions,
//                          definitions_allocated);
//                 free_types(data, definitions_allocated);
//                 return false;
//             }
//         } else if (strcmp(trimmed, "$(StartParticles)") == 0) {
//             particles_def_section = true;
//             bool n_particles_parsed = false;
//             size_t particles_allocated = 0;
//             bool end_particles_found = false;

//             while (fgets(parser->line, sizeof(parser->line), parser->file)) {
//                 parser->line_number++;

//                 char *trimmed = strutil_trim(parser->line);
//                 if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

//                 if (strcmp(trimmed, "$(EndParticles)") == 0) {
//                     end_particles_found = true;
//                     break;
//                 }

//                 if (!n_particles_parsed) {
//                     if (sscanf(trimmed, " %zu ", &data->n_particles) != 1) {
//                         parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//                         snprintf(parser->err_msg, sizeof(parser->err_msg),
//                                  "Missing number of particles in $(StartParticles) on line %d",
//                                  parser->line_number);
//                         return false;
//                     }


//                     if (data->n_particles == 0) {
//                         parser->status =  DETSCAT_PARTICLES_PARSER_ERR_INVALID_INPUT;
//                         snprintf(parser->err_msg, sizeof(parser->err_msg),
//                                  "Number of particles cannot be zero.");
//                         return false;
//                     }

//                     data->particles = malloc(data->n_particles * sizeof(DetScatParticle));
//                     if (!data->particles) {
//                         parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
//                         snprintf(parser->err_msg, sizeof(parser->err_msg),
//                                  "Could not allocate array for particles.");
//                         return false;
//                     }
//                     memset(data->particles, 0, data->n_particles * sizeof(DetScatParticle));

//                     n_particles_parsed = true;
//                     continue;
//                 }

//                 char tmp_id[64];
//                 int tmp_w, tmp_r, tmp_k;
//                 double tmp_x, tmp_y, tmp_z;

//                 if (sscanf(trimmed, " %63s %d %d %d %lf %lf %lf ", tmp_id, &tmp_w, &tmp_r, &tmp_k,
//                            &tmp_x, &tmp_y, &tmp_z) != 7) {
//                     parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//                     snprintf(parser->err_msg, sizeof(parser->err_msg),
//                              "Invalid particle definition on line %d", parser->line_number);
//                     free_particles(data, particles_allocated);
//                     return false;
//                 }

//                 if (particles_allocated >= data->n_particles) {
//                     parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//                     snprintf(parser->err_msg, sizeof(parser->err_msg),
//                              "Too many particle defined.Stopped at line %d", parser->line_number);

//                     free_particles(data, particles_allocated);
//                     return false;
//                 }

//                 data->particles[particles_allocated].id = strdup(tmp_id);
//                 data->particles[particles_allocated].w = tmp_w;
//                 data->particles[particles_allocated].r = tmp_r;
//                 data->particles[particles_allocated].k = tmp_k;
//                 data->particles[particles_allocated].position.x = tmp_x;
//                 data->particles[particles_allocated].position.y = tmp_y;
//                 data->particles[particles_allocated].position.z = tmp_z;

//                 if (!data->particles[particles_allocated].id) {
//                     parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
//                     snprintf(parser->err_msg, sizeof(parser->err_msg),
//                              "Could not allocate particle on line %d", parser->line_number);
//                     free_particles(data, particles_allocated);
//                     return false;
//                 }

//                 particles_allocated++;
//             }

//             if (!end_particles_found) {
//                 parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//                 snprintf(parser->err_msg, sizeof(parser->err_msg),
//                          "Could not find $(EndParticles) until line %d", parser->line_number);
//                 free_particles(data, particles_allocated);
//                 return false;
//             }

//             if (particles_allocated != data->n_particles) {
//                 parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//                 snprintf(parser->err_msg, sizeof(parser->err_msg),
//                          "Expected %zu particles, got %zu", data->n_particles, particles_allocated);
//                 free_particles(data, particles_allocated);
//                 return false;
//             }
//         }
//     }

//     parser->eof = true;

//     if (!types_def_section || !particles_def_section) {
//         parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
//         snprintf(parser->err_msg, sizeof(parser->err_msg), "Missing section(s): %s%s",
//                  types_def_section ? "" : "$(StartDef) ", particles_def_section ? "" : "$(StartParticles)");
//         if (data->definitions) free_types(data, data->n_definitions);
//         if (data->particles) free_particles(data, data->n_particles);
//         return false;
//     }

//     parser->status = DETSCAT_PARTICLES_PARSER_OK;
//     return true;
// }

void detscat_particles_data_free(DetScatParticlesData *data) {
    if (!data) return;

    if (data->definitions) {
        for (size_t i = 0; i < data->n_definitions; ++i) {
            free(data->definitions[i].id);
            free(data->definitions[i].data_dir);
        }
        free(data->definitions);
        data->definitions = NULL;
    }

    if (data->particles) {
        for (size_t i = 0; i < data->n_particles; ++i) {
            free(data->particles[i].id);
        }
        free(data->particles);
        data->particles = NULL;
    }

    data->n_definitions = 0;
    data->n_particles = 0;
}
