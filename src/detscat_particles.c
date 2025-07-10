#include "detscat_particles.h"

#include <stdlib.h>
#include <strings.h>

#include "strutil.h"

DetScatParticlesParser *detscat_particles_parser_create(const char *file_path) {
    DetScatParticlesParser *parser = malloc(sizeof(DetScatParticlesParser));
    if (!parser) return NULL;

    parser->file = fopen(file_path, "r");
    if (!parser->file) {
        free(parser);
        return NULL;
    }

    parser->line_number = 0;
    parser->eof = false;
    parser->status = DETSCAT_PARTICLES_PARSER_OK;
    parser->line[0] = '\0';
    parser->error_message[0] = '\0';

    return parser;
}

void detscat_particles_parser_free(DetScatParticlesParser *parser) {
    if (!parser) return;

    if (parser->file) {
        fclose(parser->file);
        parser->file = NULL;
    }

    free(parser);
}

static void free_types(DetScatParticlesData *data, size_t count) {
    if (!data || !data->definitions) return;
    for (size_t i = 0; i < count; ++i) {
        free(data->definitions[i].id);
        free(data->definitions[i].data_dir);
    }
    free(data->definitions);
    data->definitions = NULL;
}

static void free_particles(DetScatParticlesData *data, size_t count) {
    if (!data || !data->particles) return;
    for (size_t i = 0; i < count; ++i) {
        free(data->particles[i].id);
    }
    free(data->particles);
    data->particles = NULL;
}

bool detscat_particles_parser_parse(DetScatParticlesParser *parser, DetScatParticlesData *data) {
    if (!parser || !data) {
        if (parser) {
            parser->status = DETSCAT_PARTICLES_PARSER_ERR_INVALID_ARG;
            snprintf(parser->error_message, sizeof(parser->error_message),
                     "Internal error: invalid argument to 'detscat_particles_parser_parse'");
        }
        return false;
    }

    memset(data, 0, sizeof(*data));

    bool definition_sec = false;
    bool particles_sec = false;

    while (fgets(parser->line, sizeof(parser->line), parser->file)) {
        parser->line_number++;

        char *trimmed = strutil_trim(parser->line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

        if (strcmp(trimmed, "$(StartDef)") == 0) {
            definition_sec = true;

            bool n_definitions_parsed = false;
            size_t definitions_allocated = 0;
            bool end_def_found = false;

            while (fgets(parser->line, sizeof(parser->line), parser->file)) {
                parser->line_number++;

                char *trimmed = strutil_trim(parser->line);
                if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

                if (strcmp(trimmed, "$(EndDef)") == 0) {
                    end_def_found = true;
                    break;
                }

                if (!n_definitions_parsed) {
                    if (sscanf(trimmed, " %zu ", &data->n_definitions) != 1) {
                        parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                        snprintf(parser->error_message, sizeof(parser->error_message),
                                 "Missing number of particle definitions on line %d",
                                 parser->line_number);
                        return false;
                    }

                    data->definitions =
                        malloc(data->n_definitions * sizeof(DetScatParticleDefinition));
                    if (!data->definitions) {
                        parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
                        snprintf(parser->error_message, sizeof(parser->error_message),
                                 "Could not allocate particle definitions array");
                        return false;
                    }
                    memset(data->definitions, 0,
                           data->n_definitions * sizeof(DetScatParticleDefinition));

                    n_definitions_parsed = true;
                    continue;
                }

                char tmp_id[64];
                char tmp_data_dir[1024];

                if (sscanf(trimmed, " %63s %1023s ", tmp_id, tmp_data_dir) != 2) {
                    parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Invalid particle definition on line %d", parser->line_number);
                    free_types(data, definitions_allocated);
                    return false;
                }

                if (definitions_allocated >= data->n_definitions) {
                    parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Too many particle definitions. Stopped at line %d",
                             parser->line_number);
                    free_types(data, definitions_allocated);
                    return false;
                }

                data->definitions[definitions_allocated].id = strdup(tmp_id);
                data->definitions[definitions_allocated].data_dir = strdup(tmp_data_dir);

                if (!data->definitions[definitions_allocated].id ||
                    !data->definitions[definitions_allocated].data_dir) {
                    parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Could not allocate particle definition on line %d",
                             parser->line_number);
                    if (data->definitions[definitions_allocated].id)
                        free(data->definitions[definitions_allocated].id);
                    if (data->definitions[definitions_allocated].data_dir)
                        free(data->definitions[definitions_allocated].data_dir);
                    free_types(data, definitions_allocated);
                    return false;
                }

                definitions_allocated++;
            }

            if (!end_def_found) {
                parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Could not find $(EndDef) until line %d", parser->line_number);
                free_types(data, definitions_allocated);
                return false;
            }

            if (definitions_allocated != data->n_definitions) {
                parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Expected %zu particle definitions, got %zu", data->n_definitions,
                         definitions_allocated);
                free_types(data, definitions_allocated);
                return false;
            }
        } else if (strcmp(trimmed, "$(StartParticles)") == 0) {
            particles_sec = true;
            bool n_particles_parsed = false;
            size_t particles_allocated = 0;
            bool end_particles_found = false;

            while (fgets(parser->line, sizeof(parser->line), parser->file)) {
                parser->line_number++;

                char *trimmed = strutil_trim(parser->line);
                if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

                if (strcmp(trimmed, "$(EndParticles)") == 0) {
                    end_particles_found = true;
                    break;
                }

                if (!n_particles_parsed) {
                    if (sscanf(trimmed, " %zu ", &data->n_particles) != 1) {
                        parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                        snprintf(parser->error_message, sizeof(parser->error_message),
                                 "Missing number of particles in $(StartParticles) on line %d",
                                 parser->line_number);
                        return false;
                    }

                    data->particles = malloc(data->n_particles * sizeof(DetScatParticle));
                    if (!data->particles) {
                        parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
                        snprintf(parser->error_message, sizeof(parser->error_message),
                                 "Could not allocate array for particles.");
                        return false;
                    }
                    memset(data->particles, 0, data->n_particles * sizeof(DetScatParticle));

                    n_particles_parsed = true;
                    continue;
                }

                char tmp_id[64];
                double tmp_x, tmp_y, tmp_z;

                if (sscanf(trimmed, " %63s %lf %lf %lf ", tmp_id, &tmp_x, &tmp_y, &tmp_z) != 4) {
                    parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Invalid particle definition on line %d", parser->line_number);
                    free_particles(data, particles_allocated);
                    return false;
                }

                if (particles_allocated >= data->n_particles) {
                    parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Too many particle defined.Stopped at line %d",
                             parser->line_number);

                    free_particles(data, particles_allocated);
                    return false;
                }

                data->particles[particles_allocated].id = strdup(tmp_id);
                data->particles[particles_allocated].position.x = tmp_x;
                data->particles[particles_allocated].position.y = tmp_y;
                data->particles[particles_allocated].position.z = tmp_z;

                if (!data->particles[particles_allocated].id) {
                    parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
                    snprintf(parser->error_message, sizeof(parser->error_message),
                             "Could not allocate particle on line %d", parser->line_number);
                    free_particles(data, particles_allocated);
                    return false;
                }

                particles_allocated++;
            }

            if (!end_particles_found) {
                parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Could not find $(EndParticles) until line %d", parser->line_number);
                free_particles(data, particles_allocated);
                return false;
            }

            if (particles_allocated != data->n_particles) {
                parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Expected %zu particles, got %zu", data->n_particles, particles_allocated);
                free_particles(data, particles_allocated);
                return false;
            }
        }
    }

    parser->eof = true;

    if (!definition_sec || !particles_sec) {
        parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
        snprintf(parser->error_message, sizeof(parser->error_message), "Missing section(s): %s%s",
                 definition_sec ? "" : "$(StartDef) ", particles_sec ? "" : "$(StartParticles)");
        if (data->definitions) free_types(data, data->n_definitions);
        if (data->particles) free_particles(data, data->n_particles);
        return false;
    }

    parser->status = DETSCAT_PARTICLES_PARSER_OK;
    return true;
}

void detscat_particles_free(DetScatParticlesData *data) {
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
