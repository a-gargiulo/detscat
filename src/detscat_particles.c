#include "detscat_particles.h"

#include <assert.h>
#include <stdlib.h>
#include <strings.h>

#include "strutil.h"

bool detscat_prt_parser_init(DetScatPrtParser *parser, const char *file_path) {
    assert(parser != NULL);
    assert(file_path != NULL && file_path[0] != '\0');

    parser->file = fopen(file_path, "r");
    if (!parser->file) return false; 

    parser->line_number = 0;
    parser->eof = false;
    parser->status = DETSCAT_PRT_PARSER_OK;
    parser->line[0] = '\0';
    parser->errmsg[0] = '\0';

    return parser;
}

void detscat_prt_parser_close(DetScatPrtParser *parser) {
    assert(parser != NULL);

    if (parser->file) {
        fclose(parser->file);
        parser->file = NULL;
    }

    parser->line_number = 0;
    parser->status = DETSCAT_PRT_PARSER_OK;
    parser->eof = false;
    parser->line[0] = '\0';
    parser->errmsg[0] = '\0';
}

static void types_free(DetScatPrtData *prt, size_t count) {
    if (!prt || !prt->types) return;

    for (size_t i = 0; i < count; ++i) {
        free(prt->types[i].type_id);
        prt->types[i].type_id = NULL;

        free(prt->types[i].data_dir);
        prt->types[i].data_dir = NULL;
    }

    free(prt->types);
}

static void particles_free(DetScatPrtData *prt, size_t count) {
    if (!prt || !prt->particles) return;

    for (size_t i = 0; i < count; ++i) {
        free(prt->particles[i].type_id);
        prt->particles[i].type_id = NULL;

        memset(&prt->particles[i].position, 0, sizeof(prt->particles[i].position));
        memset(&prt->particles[i].case_id, 0, sizeof(prt->particles[i].case_id));
    }

    free(prt->particles);
}

bool detscat_prt_parser_load(DetScatPrtParser *parser, DetScatPrtData *prt) {
    assert(parser != NULL);
    assert(prt != NULL);

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
    bool types_parsed = false;
    bool particles_parsed = false;

    while (fgets(parser->line, sizeof(parser->line), parser->file)) {
        parser->line_number++;

        char *trimmed = strutil_trim(parser->line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

        switch (state) {
            case STATE_INITIAL:
                state = STATE_WAIT_SECTION;
                // fall through

            case STATE_WAIT_SECTION:
                if (strcmp(trimmed, "$(StartDef)") == 0) {
                    if (types_parsed) {
                        parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                        snprintf(parser->errmsg, sizeof(parser->errmsg),
                                 "Duplicate $(StartDef) at line %d",
                                 parser->line_number);
                        state = STATE_ERROR;
                        break;
                    }
                    state = STATE_PARSE_TYPES_META;
                    continue;
                } else if (strcmp(trimmed, "$(StartParticles)") == 0) {
                    if (particles_parsed) {
                        parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                        snprintf(parser->errmsg, sizeof(parser->errmsg),
                                 "Duplicate $(StartParticles) at line %d",
                                 parser->line_number);
                        state = STATE_ERROR;
                        break;
                    }
                    state = STATE_PARSE_PARTICLES_META;
                    continue;
                }
                break;

            case STATE_PARSE_TYPES_META:
                if (sscanf(trimmed, " %zu ", &prt->n_types) != 1 || 
                    prt->n_types == 0) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                            "Invalid or missing number of particle type "
                            "definitions at line %d",
                             parser->line_number);
                    state = STATE_ERROR;
                    break;
                }
                prt->types = calloc(prt->n_types, sizeof(DetScatPrtTypeDef));
                if (!prt->types) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_ALLOC;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                            "Memory allocation failed for particle type "
                            "definitions");
                    state = STATE_ERROR;
                    break;
                }
                state = STATE_PARSE_TYPES_DEF;
                break;

            case STATE_PARSE_TYPES_DEF: {
                if (strcmp(trimmed, "$(StartParticles)") == 0) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                            "Missing $(EndDef) before $(StartParticles) at "
                            "line %d",
                             parser->line_number);
                    state = STATE_ERROR;
                    break;
                }

                if (strcmp(trimmed, "$(EndDef)") == 0) {
                    if (types_allocated != prt->n_types) {
                        parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                        snprintf(parser->errmsg, sizeof(parser->errmsg),
                                "Expected %zu particle type definitions, "
                                "got %zu",
                                 prt->n_types, types_allocated);
                        state = STATE_ERROR;
                        break;
                    }
                    types_parsed = true;
                    state = STATE_WAIT_SECTION;
                    break;
                }

                char type_id[DETSCAT_PRT_TYPEID_MAX];    //size 128
                char data_dir[DETSCAT_PRT_DATADIR_MAX];  //size 512

                // truncates string in case of buffer overflow
                // 127 = 128 - 1 (null terminator) 
                // 511 = 512 - 1 (null terminator) 
                if (sscanf(trimmed, " %127s %511s ", type_id, data_dir) != 2) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Invalid particle definition at line %d",
                             parser->line_number);
                    state = STATE_ERROR;
                    break;
                }

                if (types_allocated >= prt->n_types) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                            "Too many particle type definitions. Stopped "
                            "at line %d",
                             parser->line_number);
                    state = STATE_ERROR;
                    break;
                }

                prt->types[types_allocated].type_id = strutil_strdup(type_id);
                prt->types[types_allocated].data_dir = strutil_strdup(strutil_normpath(data_dir));

                if (!prt->types[types_allocated].type_id ||
                    !prt->types[types_allocated].data_dir) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_ALLOC;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                            "Memory allocation failed for particle type "
                            "definition at line %d",
                             parser->line_number);
                    state = STATE_ERROR;
                    break;
                }

                types_allocated++;
                break;
            }

            case STATE_PARSE_PARTICLES_META:
                if (sscanf(trimmed, "%zu", &prt->n_particles) != 1 ||
                    prt->n_particles == 0) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                    snprintf(
                        parser->errmsg, sizeof(parser->errmsg),
                        "Invalid or missing number of particles at line %d",
                        parser->line_number);
                    state = STATE_ERROR;
                    break;
                }
                prt->particles = calloc(prt->n_particles, sizeof(DetScatPrtDef));
                if (!prt->particles) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_ALLOC;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Memory allocation failed for particles");
                    state = STATE_ERROR;
                    break;
                }
                state = STATE_PARSE_PARTICLES_DEF;
                break;

            case STATE_PARSE_PARTICLES_DEF: {
                if (strcmp(trimmed, "$(StartDef)") == 0) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Missing $(EndParticles) before $(StartDef) at line %d",
                             parser->line_number);
                    state = STATE_ERROR;
                    break;
                }

                if (strcmp(trimmed, "$(EndParticles)") == 0) {
                    if (particles_allocated != prt->n_particles) {
                        parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                        snprintf(parser->errmsg, sizeof(parser->errmsg),
                                 "Expected %zu particles, got %zu",
                                 prt->n_particles, particles_allocated);
                        state = STATE_ERROR;
                        break;
                    }
                    particles_parsed = true;
                    state = STATE_WAIT_SECTION;
                    break;
                }

                char type_id[DETSCAT_PRT_TYPEID_MAX];
                double x, y, z;
                int w, r, k;

                // truncates string in case of buffer overflow
                // 127 = 128 - 1 (null terminator) 
                if (sscanf(trimmed, " %127s %d %d %d %lf %lf %lf ", type_id, &w,
                           &r, &k, &x, &y, &z) != 7) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Invalid particle definition at line %d",
                             parser->line_number);
                    state = STATE_ERROR;
                    break;
                }
                
                if (particles_allocated >= prt->n_particles) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Too many particles defined. Stopped at line %d",
                             parser->line_number);
                    state = STATE_ERROR;
                    break;
                }
                DetScatPrtDef *p = &prt->particles[particles_allocated];
                p->type_id = strutil_strdup(type_id);
                p->case_id.w = w;
                p->case_id.r = r;
                p->case_id.k = k;
                p->position.x = x;
                p->position.y = y;
                p->position.z = z;
                if (!p->type_id) {
                    parser->status = DETSCAT_PRT_PARSER_ERR_ALLOC;
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Memory allocation failed for particle at line %d",
                             parser->line_number);
                    state = STATE_ERROR;
                    break;
                }
                particles_allocated++;
                break;
            }

            case STATE_ERROR:
                types_free(prt, types_allocated);
                prt->types = NULL;
                particles_free(prt, particles_allocated);
                prt->particles = NULL;
                prt->n_types = 0;
                prt->n_particles = 0;
                return false;
        }
    }

    parser->eof = true;

    if (parser->eof && state == STATE_ERROR)
    {
        types_free(prt, types_allocated);
        prt->types = NULL;
        particles_free(prt, particles_allocated);
        prt->particles = NULL;
        prt->n_types = 0;
        prt->n_particles = 0;
        return false;
    }

    if (!types_parsed && state == STATE_PARSE_TYPES_DEF) {
        parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Unexpected end of file: missing $(EndDef)");
        types_free(prt, types_allocated);
        prt->types = NULL;
        particles_free(prt, particles_allocated);
        prt->particles = NULL;
        prt->n_types = 0;
        prt->n_particles = 0;
        return false;
    }

    if (!particles_parsed && state == STATE_PARSE_PARTICLES_DEF) {
        parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Unexpected end of file: missing $(EndParticles)");
        types_free(prt, types_allocated);
        prt->types = NULL;
        particles_free(prt, particles_allocated);
        prt->particles = NULL;
        prt->n_types = 0;
        prt->n_particles = 0;
        return false;
    }

    // Final check: were both sections parsed?
    if (!types_parsed || !particles_parsed) {
        parser->status = DETSCAT_PRT_PARSER_ERR_FORMAT;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Missing section: %s",
                 !types_parsed? "$(StartDef)" : "$(StartParticles)");
        types_free(prt, types_allocated);
        prt->types = NULL;
        particles_free(prt, particles_allocated);
        prt->particles = NULL;
        prt->n_types = 0;
        prt->n_particles = 0;
        return false;
    }

    parser->status = DETSCAT_PRT_PARSER_OK;
    return true;
}

void detscat_prt_free(DetScatPrtData *prt) {
    if (!prt) return;

    types_free(prt, prt->n_types);
    prt->types = NULL;

    particles_free(prt, prt->n_particles);
    prt->particles = NULL;

    prt->n_types = 0;
    prt->n_particles = 0;
}
