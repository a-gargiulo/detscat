#include "detscat_particles.h"

#include <assert.h>
#include <stdlib.h>
#include <strings.h>

#include "strutil.h"

DetScatParticlesParser *detscat_particles_parser_create(
    const char *particles_def_file_path) {
    assert(particles_def_file_path != NULL &&
           particles_def_file_path[0] != '\0');

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
        pr_data->types[i].type_id = NULL;

        free(pr_data->types[i].data_dir);
        pr_data->types[i].data_dir = NULL;
    }

    free(pr_data->types);
    // pr_data->types = NULL;

    return;
}

static void free_particles(DetScatParticlesData *pr_data, size_t count) {
    if (!pr_data || !pr_data->particles) return;

    for (size_t i = 0; i < count; ++i) {
        free(pr_data->particles[i].type_id);
        pr_data->particles[i].type_id = NULL;

        memset(&pr_data->particles[i].position, 0, sizeof(pr_data->particles[i].position));
        memset(&pr_data->particles[i].case_id, 0, sizeof(pr_data->particles[i].case_id));
    }

    free(pr_data->particles);
    // pr_data->particles = NULL;

    return;
}

bool detscat_particles_parser_parse(DetScatParticlesParser *pr_parser,
                                    DetScatParticlesData *pr_data) {
    assert(pr_parser != NULL);
    assert(pr_data != NULL);

    typedef enum {

        STATE_INITIAL,
        STATE_WAIT_SECTION,
        STATE_PARSE_TYPES_META,
        STATE_PARSE_TYPES_DEF,
        STATE_PARSE_PARTICLES_META,
        STATE_PARSE_PARTICLES_DEF,
        STATE_ERROR

    } ParserState;

    ParserState state = STATE_INITIAL;
    size_t types_allocated = 0;
    size_t particles_allocated = 0;
    bool parsed_types = false;
    bool parsed_particles = false;

    while (fgets(pr_parser->line, sizeof(pr_parser->line), pr_parser->file)) {
        pr_parser->line_number++;

        char *trimmed = strutil_trim(pr_parser->line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

        switch (state) {
            case STATE_INITIAL:
                state = STATE_WAIT_SECTION;
                // fall through

            case STATE_WAIT_SECTION:
                if (strcmp(trimmed, "$(StartDef)") == 0) {
                    if (parsed_types) {
                        pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                        snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                                 "Duplicate $(StartDef) at line %d",
                                 pr_parser->line_number);
                        state = STATE_ERROR;
                        break;
                    }
                    state = STATE_PARSE_TYPES_META;
                    continue;
                } else if (strcmp(trimmed, "$(StartParticles)") == 0) {
                    if (parsed_particles) {
                        pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                        snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                                 "Duplicate $(StartParticles) at line %d",
                                 pr_parser->line_number);
                        state = STATE_ERROR;
                        break;
                    }
                    state = STATE_PARSE_PARTICLES_META;
                    continue;
                }
                break;

            case STATE_PARSE_TYPES_META:
                if (sscanf(trimmed, " %zu ", &pr_data->n_types) != 1 ||
                    pr_data->n_types == 0) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Invalid or missing number of type definitions on "
                             "line %d",
                             pr_parser->line_number);
                    state = STATE_ERROR;
                    break;
                }
                pr_data->types =
                    calloc(pr_data->n_types, sizeof(DetScatParticleTypeDef));
                if (!pr_data->types) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Allocation failure for type definitions");
                    state = STATE_ERROR;
                    break;
                }
                state = STATE_PARSE_TYPES_DEF;
                break;

            case STATE_PARSE_TYPES_DEF: {
                if (strcmp(trimmed, "$(StartParticles)") == 0) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Missing $(EndDef) before $(StartParticles) at line %d",
                             pr_parser->line_number);
                    state = STATE_ERROR;
                    break;
                }

                if (strcmp(trimmed, "$(EndDef)") == 0) {
                    if (types_allocated != pr_data->n_types) {
                        pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                        snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                                 "Expected %zu type definitions, got %zu",
                                 pr_data->n_types, types_allocated);
                        state = STATE_ERROR;
                        break;
                    }
                    parsed_types = true;
                    state = STATE_WAIT_SECTION;
                    break;
                }

                char type_id[DETSCAT_PARTICLES_TYPE_ID_MAX];
                char data_dir[DETSCAT_PARTICLES_DATA_DIR_MAX];

                if (sscanf(trimmed, " %127s %511s ", type_id, data_dir) != 2) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Invalid particle definition on line %d",
                             pr_parser->line_number);
                    state = STATE_ERROR;
                    break;
                }

                if (types_allocated >= pr_data->n_types) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Too many type definitions. Stopped at line %d",
                             pr_parser->line_number);
                    state = STATE_ERROR;
                    break;
                }

                pr_data->types[types_allocated].type_id = strdup(type_id);
                pr_data->types[types_allocated].data_dir =
                    strdup(strutil_normpath(data_dir));

                if (!pr_data->types[types_allocated].type_id ||
                    !pr_data->types[types_allocated].data_dir) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Allocation failed at line %d",
                             pr_parser->line_number);
                    state = STATE_ERROR;
                    break;
                }

                types_allocated++;
                break;
            }

            case STATE_PARSE_PARTICLES_META:
                if (sscanf(trimmed, "%zu", &pr_data->n_particles) != 1 ||
                    pr_data->n_particles == 0) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(
                        pr_parser->err_msg, sizeof(pr_parser->err_msg),
                        "Invalid or missing number of particles on line %d",
                        pr_parser->line_number);
                    state = STATE_ERROR;
                    break;
                }
                pr_data->particles =
                    calloc(pr_data->n_particles, sizeof(DetScatParticleDef));
                if (!pr_data->particles) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Allocation failure for particles");
                    state = STATE_ERROR;
                    break;
                }
                state = STATE_PARSE_PARTICLES_DEF;
                break;

            case STATE_PARSE_PARTICLES_DEF: {
                if (strcmp(trimmed, "$(StartDef)") == 0) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Missing $(EndParticles) before $(StartDef) at line %d",
                             pr_parser->line_number);
                    state = STATE_ERROR;
                    break;
                }

                if (strcmp(trimmed, "$(EndParticles)") == 0) {
                    if (particles_allocated != pr_data->n_particles) {
                        pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                        snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                                 "Expected %zu particles, got %zu",
                                 pr_data->n_particles, particles_allocated);
                        state = STATE_ERROR;
                        break;
                    }
                    parsed_particles = true;
                    state = STATE_WAIT_SECTION;
                    break;
                }

                char type_id[DETSCAT_PARTICLES_TYPE_ID_MAX];
                double x, y, z;
                int w, r, k;

                if (sscanf(trimmed, " %127s %d %d %d %lf %lf %lf ", type_id, &w,
                           &r, &k, &x, &y, &z) != 7) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Invalid particle definition on line %d",
                             pr_parser->line_number);
                    state = STATE_ERROR;
                    break;
                }
                if (particles_allocated >= pr_data->n_particles) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Too many particle defined. Stopped at line %d",
                             pr_parser->line_number);
                    state = STATE_ERROR;
                    break;
                }
                DetScatParticleDef *p =
                    &pr_data->particles[particles_allocated];
                p->type_id = strdup(type_id);
                p->case_id.w = w;
                p->case_id.r = r;
                p->case_id.k = k;
                p->position.x = x;
                p->position.y = y;
                p->position.z = z;
                if (!p->type_id) {
                    pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_ALLOC;
                    snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                             "Failed allocation for particle at line %d",
                             pr_parser->line_number);
                    state = STATE_ERROR;
                    break;
                }
                particles_allocated++;
                break;
            }

            case STATE_ERROR:
                free_types(pr_data, types_allocated);
                pr_data->types = NULL;
                free_particles(pr_data, particles_allocated);
                pr_data->particles = NULL;
                pr_data->n_types = 0;
                pr_data->n_particles = 0;
                return false;
        }  // switch
    }  // while

    pr_parser->eof = true;

    if (pr_parser->eof && state == STATE_ERROR)
    {
        free_types(pr_data, types_allocated);
        pr_data->types = NULL;
        free_particles(pr_data, particles_allocated);
        pr_data->particles = NULL;
        pr_data->n_types = 0;
        pr_data->n_particles = 0;
        return false;
    }

    if (!parsed_types && state == STATE_PARSE_TYPES_DEF) {
        pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
        snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                 "Unexpected end of file: missing $(EndDef)");
        free_types(pr_data, types_allocated);
        pr_data->types = NULL;
        free_particles(pr_data, particles_allocated);
        pr_data->particles = NULL;
        pr_data->n_types = 0;
        pr_data->n_particles = 0;
        return false;
    }

    if (!parsed_particles && state == STATE_PARSE_PARTICLES_DEF) {
        pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
        snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                 "Unexpected end of file: missing $(EndParticles)");
        free_types(pr_data, types_allocated);
        pr_data->types = NULL;
        free_particles(pr_data, particles_allocated);
        pr_data->particles = NULL;
        pr_data->n_types = 0;
        pr_data->n_particles = 0;
        return false;
    }

    // Final check: were both sections parsed?
    if (!parsed_types || !parsed_particles) {
        pr_parser->status = DETSCAT_PARTICLES_PARSER_ERR_FORMAT;
        snprintf(pr_parser->err_msg, sizeof(pr_parser->err_msg),
                 "Missing section: %s",
                 !parsed_types ? "$(StartDef)" : "$(StartParticles)");
        free_types(pr_data, types_allocated);
        pr_data->types = NULL;
        free_particles(pr_data, particles_allocated);
        pr_data->particles = NULL;
        pr_data->n_types = 0;
        pr_data->n_particles = 0;
        return false;
    }

    pr_parser->status = DETSCAT_PARTICLES_PARSER_OK;
    return true;
}

void detscat_particles_data_free(DetScatParticlesData *pr_data) {
    if (!pr_data) return;

    free_types(pr_data, pr_data->n_types);
    pr_data->types = NULL;

    free_particles(pr_data, pr_data->n_particles);
    pr_data->particles = NULL;

    pr_data->n_types = 0;
    pr_data->n_particles = 0;

    return;
}
