#include "detscat_parser.h"


#include "detscat_limits.h"
#include "detscat_str.h"


#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>






#include "detscat_cfg.h"
#include "detscat_ddscat.h"
#include "detscat_log.h"
#include "detscat_prt.h"

//------------------------------------------------------------------------------
// Opaque parser struct
//------------------------------------------------------------------------------

struct DetScatParser {
    FILE                   *stream;
    void                   *context;
    Str                     current_line;
    int                     line_number;
    DetScatParserStatus     status;
    DetScatParserType       type;
    bool                    eof;
    char                    error_message[256];
};

//------------------------------------------------------------------------------
// Static helpers
//------------------------------------------------------------------------------
static const uint32_t expected_magic[] = {
    [DETSCAT_CFG] = DETSCAT_CFG_MAGIC,
    [DETSCAT_PRT] = DETSCAT_PRT_MAGIC,
};

static bool detscat_parser_parse_cfg_line(DetScatParser *parser,
                                          DetScatParserCfgContext *ctx);

static bool detscat_parser_parse_prt_line(DetScatParser *parser,
                                          DetScatParserPrtContext *ctx); 

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------
DetScatParser *detscat_parser_create(DetScatParserType type) {
    assert(type >= 0 && type < DETSCAT_PARSER_TYPE_COUNT);

    DetScatParser *parser = calloc(1, sizeof(*parser));
    if (!parser) return NULL;

    if (!detscat_str_init(&parser->current_line) ||
        !detscat_str_reserve(&parser->current_line, DETSCAT_PARSER_LINE_INIT)) {
        detscat_str_free(&parser->current_line);
        free(parser);
        parser = NULL;
        return NULL;
    }

    parser->status = DETSCAT_PARSER_OK;
    parser->type = type;

    return parser;
}

void detscat_parser_destroy(DetScatParser **parser) {
    if (!parser || !*parser) return;

    if ((*parser)->stream) {
        fclose((*parser)->stream);
    }

    detscat_str_free(&(*parser)->current_line);

    free(*parser);
    *parser = NULL;
}

bool detscat_parser_init(DetScatParser *parser, const char *file_path,
                         void *context) {
    assert(parser != NULL);
    assert(context != NULL);

    if (!file_path || !*file_path) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_ARGUMENT,
                         "Invalid file path for stream initialization");
        return false;
    }

    errno = 0;
    parser->stream = fopen(file_path, "r");
    if (!parser->stream) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FILE,
                         "Could not open '%s': %s", file_path,
                         strerror(errno));
        return false;
    }
    
    assert(parser->type >=0 && parser->type < DETSCAT_PARSER_TYPE_COUNT);

    uint32_t expected = expected_magic[parser->type];
    uint32_t actual = context ? *(uint32_t *)context : 0;

    if (actual != expected) {
        fclose(parser->stream);
        parser->stream = NULL;
        // TODO: Print the parser type as a string
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_ARGUMENT,
                         "Invalid context for specified parser type");
        return false;
    }
    parser->context = context;

    return true;
}

bool detscat_parser_reset(DetScatParser *parser, const char *file_path,
                          void *context) {
    assert(parser != NULL);
    assert(context != NULL);

    if (!file_path || !*file_path) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_ARGUMENT,
                         "Invalid file path for stream resetting");
        return false;
    }

    if (parser->stream) {
        fclose(parser->stream);
        parser->stream = NULL;
    }

    detscat_str_free(&parser->current_line);
    if (!detscat_str_init(&parser->current_line) ||
        !detscat_str_reserve(&parser->current_line, DETSCAT_PARSER_LINE_INIT)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Memory allocation for parser line buffer failed");
        return false;
    }

    assert(parser->type >= 0 && parser->type < DETSCAT_PARSER_TYPE_COUNT);

    uint32_t expected = expected_magic[parser->type];
    uint32_t actual = context ? *(uint32_t *)context : 0;

    if (actual != expected) {
        // TODO: Print the parser type as a string
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_ARGUMENT,
                         "Invalid context for specified parser type");
        return false;
    }
    parser->context = context;

    errno = 0;
    parser->stream = fopen(file_path, "r");
    if (!parser->stream) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FILE,
                         "Could not open '%s': %s", file_path, strerror(errno));
        return false;
    }

    parser->line_number = 0;
    parser->status = DETSCAT_PARSER_OK;
    parser->eof = false;
    parser->error_message[0] = '\0';

    return true;
}

bool detscat_parser_next_line(DetScatParser *parser) {
    assert(parser != NULL);

    if (!parser->stream) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_STREAM,
                         "Stream is not open");
        return false;
    }

    if (!parser->current_line.data) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Line buffer is uninitialized");
        return false;
    }

    if (!parser->context) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_CONTEXT,
                         "Context is uninitialized");
        return false;
    }

    if (parser->eof) {
        parser->status = DETSCAT_PARSER_EOF;
        return false;
    }

    parser->current_line.length = 0;
    parser->current_line.data[0] = '\0';

    int ch;
    while ((ch = fgetc(parser->stream)) != EOF) {
        if (parser->current_line.length >= DETSCAT_PARSER_LINE_MAX) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_OVERFLOW,
                             "Line exceeds maximum allowed length (%zu bytes)",
                             DETSCAT_PARSER_LINE_MAX);
            return false;
        }

        if (!detscat_str_append_char(&parser->current_line, (char)ch)) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                             "Failed to allocate memory for line buffer");
            return false;
        }

        if (ch == '\n') break;
    }

    if (ch == EOF) {
        if (parser->current_line.length == 0) {
            parser->eof = true;
            parser->status = DETSCAT_PARSER_EOF;
            return false;
        }
        parser->eof = true;
    }

    parser->line_number++;
    parser->status = DETSCAT_PARSER_OK;
    return true;
}

bool detscat_parser_parse_line(DetScatParser *parser) {
    assert(parser != NULL);

    assert(parser->type >= 0 && parser->type < DETSCAT_PARSER_TYPE_COUNT);
    switch (parser->type) {
        case DETSCAT_CFG:
            return detscat_parser_parse_cfg_line(
                parser, (DetScatParserCfgContext *)parser->context);

        case DETSCAT_PRT:
            return detscat_parser_parse_prt_line(
                parser, (DetScatParserPrtContext *)parser->context);

        default:
            // TODO: Handle error more clearly
            return true;
    }
}






















static bool parse_bool_cfg(const char *value, bool *out, DetScatParser *parser,
                           const char *key) {
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);
    assert(parser != NULL);
    assert(key != NULL && key[0] != '\0');

    if (detscat_str_raw_strcasecmp(value, "true") == 0) {
        *out = true;
    } else if (detscat_str_raw_strcasecmp(value, "false") == 0) {
        *out = false;
    } else {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid bool format for '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    return true;
}

static bool parse_double_cfg(const char *value, double *out,
                             DetScatParser *parser, const char *key) {
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);
    assert(parser != NULL);
    assert(key != NULL && key[0] != '\0');

    errno = 0;
    char *endptr = NULL;
    double val = strtod(value, &endptr);
    if (errno == ERANGE && (val == HUGE_VAL || val == -HUGE_VAL)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Overflow parsing '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    if (errno == ERANGE && val == 0) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Underflow parsing '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    if (endptr == value || *endptr != '\0') {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid double format for '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    *out = val;
    return true;
}

static bool parse_int_cfg(const char *value, int *out, DetScatParser *parser,
                          const char *key) {
    assert(parser != NULL);
    assert(value != NULL && *value);
    assert(key != NULL && *key);
    assert(out != NULL);

    errno = 0;
    char *endptr = NULL;
    long val = strtol(value, &endptr, 10);

    if ((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Overflow/underflow parsing '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    if (endptr == value || *endptr != '\0') {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid integer format for '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    // check if casting long to int is safe
    if (val < INT_MIN || val > INT_MAX) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Integer value out of range for '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    *out = (int)val;
    return true;
}


static bool detscat_prt_parse_type(DetScatParser *parser, DetScatPrtType *type,
                                   const char *line) {
    assert(parser != NULL);
    assert(type != NULL);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid particle type definition at line %d",
                         parser->line_number);
        return false;
    }

    char *copy = detscat_str_raw_strdup(line);
    if (!copy) return false;

    char *token1 = strtok(copy, " \t");
    char *token2 = strtok(NULL, " \t");
    char *token3 = strtok(NULL, " \t");

    if (!token1 || !token2 || token3) {
        parser->status = DETSCAT_PARSER_ERR_FORMAT;
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid particle type definition at line %d",
                         parser->line_number);
        goto error_cleanup;
    }

    if (strlen(token1) >= DETSCAT_PRT_TYPEID_MAX ||
        strlen(token2) >= DETSCAT_PRT_PATH_MAX) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Particle type or data path out of range at line %d",
                         parser->line_number);
        goto error_cleanup;
    }

    if (!detscat_str_set(&type->type_id, token1) ||
        !detscat_str_set(&type->data_dir, token2)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not set data for particle type at line %d",
                         parser->line_number);
        goto error_cleanup;
    }
    detscat_str_raw_normpath(type->data_dir.data);

    free(copy);
    return true;

error_cleanup:
    free(copy);
    return false;
}

static bool parse_int_prt(const char *str, int *out) {
    assert(out != NULL);
    if (!str || !*str) return false;

    errno = 0;
    char *endptr;
    long val = strtol(str, &endptr, 10);

    if (endptr == str) return false;
    if (*endptr != '\0') return false;
    if ((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE) return false;
    if (val > INT_MAX || val < INT_MIN) return false;

    *out = (int)val;
    return true;
}

static bool parse_double_prt(const char *str, double *out) {
    assert(out != NULL);
    if (!str || !*str) return false;

    char *endptr;
    errno = 0;
    double val = strtod(str, &endptr);

    if (endptr == str) return false;
    if (*endptr != '\0') return false;
    if (errno == ERANGE && (val == HUGE_VAL || val == -HUGE_VAL)) return false;
    if (errno == ERANGE && val == 0) return false;

    *out = val;
    return true;
}

static bool detscat_prt_parse_particle(DetScatParser *parser,
                                       DetScatPrtParticle *particle,
                                       const char *line) {
    assert(parser != NULL);
    assert(particle != NULL);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid particle type definition at line %d",
                         parser->line_number);
        return false;
    }

    char *copy = detscat_str_raw_strdup(line);
    if (!copy) return false;

    int wrk[3];
    double xyz[3];

    char *tok = strtok(copy, " \t");
    if (!tok) {
        free(copy);
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid particle definition at line %d",
                         parser->line_number);
        return false;
    }
    if (strlen(tok) >= DETSCAT_PRT_TYPEID_MAX) {
        free(copy);
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Particle type too long at line %d",
                         parser->line_number);
        return false;
    }

    char *type_id = detscat_str_raw_strdup(tok);

    for (size_t i = 0; i < 3; ++i) {
        tok = strtok(NULL, " \t");
        if (!tok) goto error_cleanup;
        if (!parse_int_prt(tok, &wrk[i])) goto error_cleanup;
    }

    for (size_t i = 0; i < 3; ++i) {
        tok = strtok(NULL, " \t");
        if (!tok) goto error_cleanup;
        if (!parse_double_prt(tok, &xyz[i])) goto error_cleanup;
    }

    tok = strtok(NULL, " \t");
    if (tok) goto error_cleanup;

    if (!detscat_str_set(&particle->type_id, type_id)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not set particle type ID at line %d",
                         parser->line_number);
        goto error_cleanup;
    }

    particle->case_id->w = wrk[0];
    particle->case_id->r = wrk[1];
    particle->case_id->k = wrk[2];
    particle->position.x = xyz[0];
    particle->position.y = xyz[1];
    particle->position.z = xyz[2];

    free(copy);
    free(type_id);
    return true;

error_cleanup:
    free(copy);
    free(type_id);
    PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                     "Invalid particle definition at line %d",
                     parser->line_number);
    return false;
}

static bool detscat_parser_parse_prt_line(DetScatParser *parser,
                                          DetScatParserPrtContext *ctx) {
    assert(parser != NULL);
    assert(ctx != NULL);

    if (!parser->stream) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_STREAM,
                         "Stream is not open");
        return false;
    }

    if (!parser->current_line.data) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Line buffer is uninitialized");
        return false;
    }

    if (!parser->context) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_CONTEXT,
                         "Contest is uninitialized");
        return false;
    }

    DetScatPrt *prt = ctx->prt;

    char *trimmed = detscat_str_raw_trim(parser->current_line.data);
    if (trimmed[0] == '\0' || trimmed[0] == '#') return true;

    switch (ctx->state) {
        case STATE_INITIAL:
            ctx->state = STATE_WAIT_SECTION;
            // fall through

        case STATE_WAIT_SECTION:
            if (strcmp(trimmed, "$(StartTypes)") == 0) {
                if (ctx->types_parsed) {
                    PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                                     "Duplicate $(StartTypes) at line %d",
                                     parser->line_number);
                    ctx->state = STATE_ERROR;
                    goto error_cleanup;
                }
                ctx->state = STATE_PARSE_TYPES_META;
                return true;
            } else if (strcmp(trimmed, "$(StartParticles)") == 0) {
                if (ctx->particles_parsed) {
                    PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                                     "Duplicate $(StartParticles) at line %d",
                                     parser->line_number);
                    ctx->state = STATE_ERROR;
                    goto error_cleanup;
                }
                ctx->state = STATE_PARSE_PARTICLES_META;
                return true;
            }
            return true;

        case STATE_PARSE_TYPES_META:
            if (sscanf(trimmed, " %zu ", &prt->n_types) != 1 ||
                prt->n_types == 0) {
                PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                                 "Invalid or missing number of particle types "
                                 "at line %d",
                                 parser->line_number);
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }
            prt->types = calloc(prt->n_types, sizeof(*(prt->types)));
            if (!prt->types) {
                PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                                 "Memory allocation failed for particle types");
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }
            ctx->state = STATE_PARSE_TYPES_DEF;
            return true;

        case STATE_PARSE_TYPES_DEF:
            if (strcmp(trimmed, "$(StartParticles)") == 0) {
                PARSER_SET_ERROR(
                    parser, DETSCAT_PARSER_ERR_FORMAT,
                    "Missing $(EndTypes) before $(StartParticles) at "
                    "line %d",
                    parser->line_number);
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }

            if (strcmp(trimmed, "$(EndTypes)") == 0) {
                if (ctx->types_allocated != prt->n_types) {
                    PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                                     "Expected %zu particle type definitions, "
                                     "got %zu",
                                     prt->n_types, ctx->types_allocated);
                    ctx->state = STATE_ERROR;
                    goto error_cleanup;
                }
                ctx->types_parsed = true;
                ctx->state = STATE_WAIT_SECTION;
                return true;
            }

            if (ctx->types_allocated >= prt->n_types) {
                parser->status = DETSCAT_PARSER_ERR_FORMAT;
                PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                                 "Too many particle type definitions. Stopped "
                                 "at line %d",
                                 parser->line_number);
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }

            if (!detscat_str_init(&prt->types[ctx->types_allocated].type_id) ||
                !detscat_str_init(&prt->types[ctx->types_allocated].data_dir) ||
                !detscat_str_reserve(&prt->types[ctx->types_allocated].type_id,
                                     DETSCAT_PRT_TYPEID_INIT) ||
                !detscat_str_reserve(&prt->types[ctx->types_allocated].data_dir,
                                     DETSCAT_PRT_PATH_INIT)) {
                PARSER_SET_ERROR(
                    parser, DETSCAT_PARSER_ERR_MEMORY,
                    "Memory allocation failed for particle type ID "
                    "or data path at line %d",
                    parser->line_number);
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }

            if (!detscat_prt_parse_type(
                    parser, &prt->types[ctx->types_allocated], trimmed)) {
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }

            ctx->types_allocated++;

            return true;

        case STATE_PARSE_PARTICLES_META:
            if (sscanf(trimmed, "%zu", &prt->n_particles) != 1 ||
                prt->n_particles == 0) {
                PARSER_SET_ERROR(
                    parser, DETSCAT_PARSER_ERR_FORMAT,
                    "Invalid or missing number of particles at line %d",
                    parser->line_number);
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }
            prt->particles =
                calloc(prt->n_particles, sizeof(*(prt->particles)));
            if (!prt->particles) {
                PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                                 "Memory allocation failed for particles");
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }
            ctx->state = STATE_PARSE_PARTICLES_DEF;
            return true;

        case STATE_PARSE_PARTICLES_DEF:
            if (strcmp(trimmed, "$(StartTypes)") == 0) {
                PARSER_SET_ERROR(
                    parser, DETSCAT_PARSER_ERR_FORMAT,
                    "Missing $(EndParticles) before $(StartTypes) at line %d",
                    parser->line_number);
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }

            if (strcmp(trimmed, "$(EndParticles)") == 0) {
                if (ctx->particles_allocated != prt->n_particles) {
                    PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                                     "Expected %zu particles, got %zu",
                                     prt->n_particles,
                                     ctx->particles_allocated);
                    ctx->state = STATE_ERROR;
                    goto error_cleanup;
                }
                ctx->particles_parsed = true;
                ctx->state = STATE_WAIT_SECTION;
                return true;
            }

            if (ctx->particles_allocated >= prt->n_particles) {
                PARSER_SET_ERROR(
                    parser, DETSCAT_PARSER_ERR_FORMAT,
                    "Too many particles defined. Stopped at line %d",
                    parser->line_number);
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }

            if (!detscat_str_init(
                    &prt->particles[ctx->particles_allocated].type_id) ||
                !detscat_str_reserve(
                    &prt->particles[ctx->particles_allocated].type_id,
                    DETSCAT_PRT_TYPEID_INIT)) {
                PARSER_SET_ERROR(
                    parser, DETSCAT_PARSER_ERR_MEMORY,
                    "Memory allocation failed for particle type ID "
                    "at line %d",
                    parser->line_number);
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }

            prt->particles[ctx->particles_allocated].case_id = calloc(
                1, sizeof(*prt->particles[ctx->particles_allocated].case_id));

            if (!detscat_prt_parse_particle(
                    parser, &prt->particles[ctx->particles_allocated],
                    trimmed)) {
                ctx->state = STATE_ERROR;
                goto error_cleanup;
            }

            ctx->particles_allocated++;
            return true;

        case STATE_ERROR:
            goto error_cleanup;

        default:
            return true;
    }

error_cleanup:
    detscat_prt_free_subset(prt, ctx->types_allocated,
                            ctx->particles_allocated);
    return false;
}






const char *detscat_parser_error_message(const DetScatParser *parser) {
    assert(parser != NULL);
    return parser->error_message;
}

char *detscat_parser_line_buffer(const DetScatParser *parser) {
    assert(parser != NULL);
    return parser->current_line.data;
}

bool detscat_parser_eof(const DetScatParser *parser) {
    assert(parser != NULL);
    return parser->eof;
}

FILE *detscat_parser_stream(const DetScatParser *parser) {
    assert(parser != NULL);
    return parser->stream;
}

void detscat_parser_set_status(DetScatParser *parser,
                               DetScatParserStatus status) {
    assert(parser != NULL);
    assert(status >= 0 && status < DETSCAT_PARSER_STATUS_COUNT);
    parser->status = status;
}

bool detscat_parser_set_error_message(DetScatParser *parser,
                                      const char *error_message) {
    assert(parser != NULL);
    assert(error_message != NULL && error_message[0] != '\0');

    if (strlen(error_message) > sizeof(parser->error_message) - 1) return false;

    snprintf(parser->error_message, sizeof(parser->error_message), "%s",
             error_message);

    return true;
}

//------------------------------------------------------------------------------
// Static helpers - IMPLEMENTATION
//------------------------------------------------------------------------------

static bool detscat_parser_parse_cfg_line(DetScatParser *parser,
                                          DetScatParserCfgContext *ctx) {
    assert(parser != NULL);
    assert(ctx != NULL);

    assert(parser->stream != NULL);
    assert(parser->current_line.data != NULL);
    assert(parser->context != NULL);

    DetScatConfig *cfg = ctx->cfg;

    char *trimmed = detscat_str_raw_trim(parser->current_line.data);
    if (trimmed[0] == '\0' || trimmed[0] == '#') return true;

    char *equals = strchr(trimmed, '=');
    if (!equals) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Missing '=' at line %d", parser->line_number);
        return false;
    }

    *equals = '\0';
    char *key = parser->current_line.data;
    char *value = equals + 1;

    key = detscat_str_raw_trim(key);
    value = detscat_str_raw_trim(value);

    if (strcmp(key, "is_polarized") == 0) {
        if (!parse_bool_cfg(value, &cfg->is_polarized, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "polarization") == 0) {
        int matched =
            sscanf(value, "[ [ %lf , %lf ] , [ %lf , %lf ] , [ %lf , %lf ] ]",
                   &cfg->polarization.x.re, &cfg->polarization.x.im,
                   &cfg->polarization.y.re, &cfg->polarization.y.im,
                   &cfg->polarization.z.re, &cfg->polarization.z.im);
        if (matched != 6) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Invalid array format for '%s' at line %d", key,
                             parser->line_number);
            return false;
        }
    } else if (strcmp(key, "wavelength_nm") == 0) {
        if (!parse_double_cfg(value, &cfg->wavelength_nm, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "pulse_energy_mj") == 0) {
        if (!parse_double_cfg(value, &cfg->pulse_energy_mj, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "pulse_width_ns") == 0) {
        if (!parse_double_cfg(value, &cfg->pulse_width_ns, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "beam_diameter_mm") == 0) {
        if (!parse_double_cfg(value, &cfg->beam_diameter_mm, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "particles_definition_file") == 0) {
        if (strlen(value) > DETSCAT_CFG_PATH_MAX - 1) {
            PARSER_SET_ERROR(
                parser, DETSCAT_PARSER_ERR_FORMAT,
                "Path too long for '%s' at line %d (max %zu chars)", key,
                parser->line_number, DETSCAT_CFG_PATH_MAX - 1);
            return false;
        }
        if (!detscat_str_set(&cfg->particles_file_path, value)) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Could not set '%s' at line %d", key,
                             parser->line_number);
            return false;
        }
    } else if (strcmp(key, "camera_center_position_m") == 0) {
        int matched = sscanf(
            value, "[ %lf , %lf , %lf ]", &cfg->camera_center_position_m.x,
            &cfg->camera_center_position_m.y, &cfg->camera_center_position_m.z);
        if (matched != 3) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Invalid array format for '%s' at line %d", key,
                             parser->line_number);
            return false;
        }
    } else if (strcmp(key, "camera_sensor_normal_vector") == 0) {
        int matched =
            sscanf(value, "[ %lf , %lf , %lf ]", &cfg->camera_sensor_normal.x,
                   &cfg->camera_sensor_normal.y, &cfg->camera_sensor_normal.z);
        if (matched != 3) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Invalid array format for '%s' at line %d", key,
                             parser->line_number);
            return false;
        }
    } else if (strcmp(key, "sensor_width_mm") == 0) {
        if (!parse_double_cfg(value, &cfg->sensor_width_mm, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "sensor_height_mm") == 0) {
        if (!parse_double_cfg(value, &cfg->sensor_height_mm, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "focal_length_mm") == 0) {
        if (!parse_double_cfg(value, &cfg->focal_length_mm, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "camera_resolution_x_px") == 0) {
        if (!parse_int_cfg(value, &cfg->camera_resolution_x_px, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "camera_resolution_y_px") == 0) {
        if (!parse_int_cfg(value, &cfg->camera_resolution_y_px, parser, key)) {
            return false;
        }
    } else {
        parser->status = DETSCAT_PARSER_ERR_UNKNOWN_KEY;
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_UNKNOWN_KEY,
                         "Unknown key '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    parser->status = DETSCAT_PARSER_OK;
    return true;
}
