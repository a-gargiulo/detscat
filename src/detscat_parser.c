#include "detscat_parser.h"

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
#include "detscat_limits.h"
#include "detscat_prt.h"
#include "detscat_str.h"

// --- Parser context signatures  ---
static const uint32_t expected_magic[] = {
    [DETSCAT_CFG] = DETSCAT_CFG_MAGIC,
    [DETSCAT_PRT] = DETSCAT_PRT_MAGIC,
};

// --- Internal helpers (PRIVATE) ---

//------------------------------------------------------------------------------
// CFG Parser
//------------------------------------------------------------------------------
static bool parse_bool_cfg(const char *value, bool *out, DetScatParser *parser,
                           const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_INVALID_ARG,
            "Invalid or empty key and/or value provided at line %d",
            parser->line_number);

        return false;
    }

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

    parser->status = DETSCAT_PARSER_OK;
    return true;
}

static bool parse_double_cfg(const char *value, double *out,
                             DetScatParser *parser, const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_INVALID_ARG,
            "Invalid or empty key and/or value provided at line %d",
            parser->line_number);

        return false;
    }

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
    parser->status = DETSCAT_PARSER_OK;
    return true;
}

static bool parse_int_cfg(const char *value, int *out, DetScatParser *parser,
                          const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_INVALID_ARG,
            "Invalid or empty key and/or value provided at line %d",
            parser->line_number);

        return false;
    }

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
    parser->status = DETSCAT_PARSER_OK;
    return true;
}

static bool parse_cplx_vec3_cfg(const char *value, ComplexVec3 *out,
                                DetScatParser *parser, const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_INVALID_ARG,
            "Invalid or empty key and/or value provided at line %d",
            parser->line_number);

        return false;
    }

    int matched = sscanf(
        value, "[ [ %lf , %lf ] , [ %lf , %lf ] , [ %lf , %lf ] ]", &out->x.re,
        &out->x.im, &out->y.re, &out->y.im, &out->z.re, &out->z.im);
    if (matched != 6) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid array format for '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    parser->status = DETSCAT_PARSER_OK;
    return true;
}

static bool parse_vec3_cfg(const char *value, Vec3 *out, DetScatParser *parser,
                           const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_INVALID_ARG,
            "Invalid or empty key and/or value provided at line %d",
            parser->line_number);

        return false;
    }

    int matched =
        sscanf(value, "[ %lf , %lf , %lf ]", &out->x, &out->y, &out->z);
    if (matched != 3) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid array format for '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    parser->status = DETSCAT_PARSER_OK;
    return true;
}

static bool parse_str_cfg(const char *value, Str *out, DetScatParser *parser,
                          const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_INVALID_ARG,
            "Invalid or empty key and/or value provided at line %d",
            parser->line_number);

        return false;
    }

    if (strlen(value) > DETSCAT_CFG_PATH_MAX - 1) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Path too long for '%s' at line %d (max %zu chars)",
                         key, parser->line_number, DETSCAT_CFG_PATH_MAX - 1);
        return false;
    }

    if (!detscat_str_set(out, value)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Could not set '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    parser->status = DETSCAT_PARSER_OK;
    return true;
}

static bool detscat_parser_split_key_value(char *line, char **key,
                                           char **value) {
    if (!line || !*line || !key || !value) return false;

    char *equals = strchr(line, '=');
    if (!equals) return false;

    *equals = '\0';

    *key = detscat_str_raw_trim(line);
    *value = detscat_str_raw_trim(equals + 1);

    if (!**key || !**value) {
        *key = NULL;
        *value = NULL;
        return false;
    }

    return true;
}

static bool detscat_parser_handle_key_cfg(DetScatConfig *cfg, const char *key,
                                          const char *value,
                                          DetScatParser *parser) {
    assert(parser && cfg);
    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_INVALID_ARG,
            "Invalid or empty key and/or value provided at line %d",
            parser->line_number);

        return false;
    }

    if (strcmp(key, "is_polarized") == 0) {
        return parse_bool_cfg(value, &cfg->is_polarized, parser, key);
    } else if (strcmp(key, "polarization") == 0) {
        return parse_cplx_vec3_cfg(value, &cfg->polarization, parser, key);
    } else if (strcmp(key, "wavelength_nm") == 0) {
        return parse_double_cfg(value, &cfg->wavelength_nm, parser, key);
    } else if (strcmp(key, "pulse_energy_mj") == 0) {
        return parse_double_cfg(value, &cfg->pulse_energy_mj, parser, key);
    } else if (strcmp(key, "pulse_width_ns") == 0) {
        return parse_double_cfg(value, &cfg->pulse_width_ns, parser, key);
    } else if (strcmp(key, "beam_diameter_mm") == 0) {
        return parse_double_cfg(value, &cfg->beam_diameter_mm, parser, key);
    } else if (strcmp(key, "particles_definition_file") == 0) {
        return parse_str_cfg(value, &cfg->particles_file_path, parser, key);
    } else if (strcmp(key, "camera_center_position_m") == 0) {
        return parse_vec3_cfg(value, &cfg->camera_center_position_m, parser,
                              key);
    } else if (strcmp(key, "camera_sensor_normal_vector") == 0) {
        return parse_vec3_cfg(value, &cfg->camera_sensor_normal, parser, key);
    } else if (strcmp(key, "sensor_width_mm") == 0) {
        return parse_double_cfg(value, &cfg->sensor_width_mm, parser, key);
    } else if (strcmp(key, "sensor_height_mm") == 0) {
        return parse_double_cfg(value, &cfg->sensor_height_mm, parser, key);
    } else if (strcmp(key, "focal_length_mm") == 0) {
        return parse_double_cfg(value, &cfg->focal_length_mm, parser, key);
    } else if (strcmp(key, "camera_resolution_x_px") == 0) {
        return parse_int_cfg(value, &cfg->camera_resolution_x_px, parser, key);
    } else if (strcmp(key, "camera_resolution_y_px") == 0) {
        return parse_int_cfg(value, &cfg->camera_resolution_y_px, parser, key);
    } else {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_UNKNOWN_KEY,
                         "Unknown key '%s' at line %d", key,
                         parser->line_number);
        return false;
    }
}

static bool detscat_parser_parse_line_cfg(DetScatParser *parser,
                                          DetScatParserCfgContext *ctx) {
    assert(parser && ctx);

    char *trimmed = detscat_str_raw_trim(parser->current_line.data);
    if (trimmed[0] == '\0' || trimmed[0] == '#') return true;

    char *key = NULL, *value = NULL;
    if (!detscat_parser_split_key_value(trimmed, &key, &value)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_SYNTAX,
                         "Missing '=' at line %d", parser->line_number);
        return false;
    }

    if (!detscat_parser_handle_key_cfg(ctx->cfg, key, value, parser))
        return false;

    parser->status = DETSCAT_PARSER_OK;
    return true;
}

//------------------------------------------------------------------------------
// PRT Parser
//------------------------------------------------------------------------------
static bool parse_double_prt(const char *str, double *out,
                             DetScatParser *parser) {
    assert(parser && out);
    if (!str || !*str) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid or empty buffer provided at line %d",
                         parser->line_number);
        return false;
    }

    char *endptr;
    errno = 0;
    double val = strtod(str, &endptr);

    if (endptr == str) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Expected a numeric value at line %d (empty or invalid input)",
            parser->line_number);
        return false;
    }
    if (*endptr != '\0') {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Unexpected trailing characters in numeric value at line %d: '%s'",
            parser->line_number, endptr);
        return false;
    }
    if (errno == ERANGE && (val == HUGE_VAL || val == -HUGE_VAL)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_OVERFLOW,
                         "Numeric value at line %d is out of range (overflow)",
                         parser->line_number);
        return false;
    }
    if (errno == ERANGE && val == 0) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_UNDERFLOW,
                         "Numeric value at line %d is too small (underflow)",
                         parser->line_number);
        return false;
    }

    *out = val;
    return true;
}

static bool parse_int_prt(const char *str, int *out, DetScatParser *parser) {
    assert(parser && out);

    if (!str || !*str) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Expected an integer value at line %d (empty or null input)",
            parser->line_number);
        return false;
    }

    errno = 0;
    char *endptr = NULL;
    long val = strtol(str, &endptr, 10);

    if (endptr == str) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Expected an integer value at line %d (invalid input: '%s')",
            parser->line_number, str);
        return false;
    }

    if (*endptr != '\0') {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Unexpected trailing characters in integer value at line %d: '%s'",
            parser->line_number, endptr);
        return false;
    }

    if ((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Integer value at line %d is out of range for 'long'",
                         parser->line_number);
        return false;
    }

    if (val > INT_MAX || val < INT_MIN) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Integer value at line %d is out of range for 'int' "
                         "(%ld is outside [%d, %d])",
                         parser->line_number, val, INT_MIN, INT_MAX);
        return false;
    }

    *out = (int)val;
    return true;
}

static bool parse_type_prt(DetScatParser *parser, DetScatPrtType *type,
                           const char *line) {
    assert(parser && type);
    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Expected a particle type definition at line %d "
                         "(empty or null input)",
                         parser->line_number);
        return false;
    }

    char *copy = detscat_str_raw_strdup(line);
    if (!copy) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_MEMORY,
            "Could not allocate memory for temporary buffer copy at line %d",
            parser->line_number);
        return false;
    }

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

static bool parse_particle_prt(DetScatParser *parser,
                               DetScatPrtParticle *particle, const char *line) {
    assert(parser && particle);
    if (!line || !*line) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Expected a particle definition at line %d (empty or null input)",
            parser->line_number);
        return false;
    }

    char *copy = detscat_str_raw_strdup(line);
    if (!copy) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Could not allocate memory for temporary buffer copy at line %d",
            parser->line_number);
        return false;
    }

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
        if (!parse_int_prt(tok, &wrk[i], parser)) goto error_cleanup;
    }

    for (size_t i = 0; i < 3; ++i) {
        tok = strtok(NULL, " \t");
        if (!tok) goto error_cleanup;
        if (!parse_double_prt(tok, &xyz[i], parser)) goto error_cleanup;
    }

    tok = strtok(NULL, " \t");
    if (tok) goto error_cleanup;

    if (!detscat_str_set(&particle->type_id, type_id)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not set particle type ID at line %d",
                         parser->line_number);
        goto error_cleanup;
    }

    particle->case_id.w = wrk[0];
    particle->case_id.r = wrk[1];
    particle->case_id.k = wrk[2];
    particle->position.x = xyz[0];
    particle->position.y = xyz[1];
    particle->position.z = xyz[2];

    free(copy);
    free(type_id);
    return true;

error_cleanup:
    free(copy);
    free(type_id);
    return false;
}

static bool detscat_parser_wait_prt(const char *line,
                                    DetScatParserPrtContext *ctx,
                                    DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Received an empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (strcmp(line, "$(StartTypes)") == 0) {
        if (ctx->types_parsed) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Duplicate $(StartTypes) at line %d",
                             parser->line_number);
            ctx->state = STATE_ERROR;
            return false;
        }
        ctx->state = STATE_PARSE_TYPES_META;
        return true;
    } else if (strcmp(line, "$(StartParticles)") == 0) {
        if (ctx->particles_parsed) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Duplicate $(StartParticles) at line %d",
                             parser->line_number);
            ctx->state = STATE_ERROR;
            return false;
        }
        ctx->state = STATE_PARSE_PARTICLES_META;
        return true;
    }
    return true;
}

static bool detscat_parser_types_meta_prt(const char *line,
                                          DetScatParserPrtContext *ctx,
                                          DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Received an empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (sscanf(line, " %zu ", &ctx->prt->n_types) != 1 ||
        ctx->prt->n_types == 0) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid or missing number of particle types "
                         "at line %d",
                         parser->line_number);
        ctx->state = STATE_ERROR;
        return false;
    }
    ctx->prt->types = calloc(ctx->prt->n_types, sizeof(*(ctx->prt->types)));
    if (!ctx->prt->types) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Memory allocation failed for particle types");
        ctx->state = STATE_ERROR;
        return false;
    }
    ctx->state = STATE_PARSE_TYPES_DEF;
    return true;
}

static bool detscat_parser_types_def_prt(const char *line,
                                         DetScatParserPrtContext *ctx,
                                         DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Received an empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (strcmp(line, "$(StartParticles)") == 0) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Missing $(EndTypes) before $(StartParticles) at "
                         "line %d",
                         parser->line_number);
        ctx->state = STATE_ERROR;
        return false;
    }

    if (strcmp(line, "$(EndTypes)") == 0) {
        if (ctx->types_allocated != ctx->prt->n_types) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Expected %zu particle type definitions, "
                             "got %zu",
                             ctx->prt->n_types, ctx->types_allocated);
            ctx->state = STATE_ERROR;
            return false;
        }
        ctx->types_parsed = true;
        ctx->state = STATE_WAIT_SECTION;
        return true;
    }

    if (ctx->types_allocated >= ctx->prt->n_types) {
        parser->status = DETSCAT_PARSER_ERR_FORMAT;
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Too many particle type definitions. Stopped "
                         "at line %d",
                         parser->line_number);
        ctx->state = STATE_ERROR;
        return false;
    }

    if (!detscat_str_init(&ctx->prt->types[ctx->types_allocated].type_id) ||
        !detscat_str_init(&ctx->prt->types[ctx->types_allocated].data_dir) ||
        !detscat_str_reserve(&ctx->prt->types[ctx->types_allocated].type_id,
                             DETSCAT_PRT_TYPEID_INIT) ||
        !detscat_str_reserve(&ctx->prt->types[ctx->types_allocated].data_dir,
                             DETSCAT_PRT_PATH_INIT)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Memory allocation failed for particle type ID "
                         "or data path at line %d",
                         parser->line_number);
        ctx->state = STATE_ERROR;
        return false;
    }

    if (!parse_type_prt(parser, &ctx->prt->types[ctx->types_allocated], line)) {
        ctx->state = STATE_ERROR;
        return false;
    }

    ctx->types_allocated++;

    return true;
}

static bool detscat_parser_particles_meta_prt(const char *line,
                                              DetScatParserPrtContext *ctx,
                                              DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Received an empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (sscanf(line, "%zu", &ctx->prt->n_particles) != 1 ||
        ctx->prt->n_particles == 0) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid or missing number of particles at line %d",
                         parser->line_number);
        ctx->state = STATE_ERROR;
        return false;
    }
    ctx->prt->particles =
        calloc(ctx->prt->n_particles, sizeof(*(ctx->prt->particles)));
    if (!ctx->prt->particles) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Memory allocation failed for particles");
        ctx->state = STATE_ERROR;
        return false;
    }
    ctx->state = STATE_PARSE_PARTICLES_DEF;
    return true;
}

static bool detscat_parser_particles_prt(const char *line,
                                         DetScatParserPrtContext *ctx,
                                         DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Received an empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (strcmp(line, "$(StartTypes)") == 0) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Missing $(EndParticles) before $(StartTypes) at line %d",
            parser->line_number);
        ctx->state = STATE_ERROR;
        return false;
    }

    if (strcmp(line, "$(EndParticles)") == 0) {
        if (ctx->particles_allocated != ctx->prt->n_particles) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Expected %zu particles, got %zu",
                             ctx->prt->n_particles, ctx->particles_allocated);
            ctx->state = STATE_ERROR;
            return false;
        }
        ctx->particles_parsed = true;
        ctx->state = STATE_WAIT_SECTION;
        return true;
    }

    if (ctx->particles_allocated >= ctx->prt->n_particles) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Too many particles defined. Stopped at line %d",
                         parser->line_number);
        ctx->state = STATE_ERROR;
        return false;
    }

    if (!detscat_str_init(
            &ctx->prt->particles[ctx->particles_allocated].type_id) ||
        !detscat_str_reserve(
            &ctx->prt->particles[ctx->particles_allocated].type_id,
            DETSCAT_PRT_TYPEID_INIT)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Memory allocation failed for particle type ID "
                         "at line %d",
                         parser->line_number);
        ctx->state = STATE_ERROR;
        return false;
    }

    if (!parse_particle_prt(
            parser, &ctx->prt->particles[ctx->particles_allocated], line)) {
        ctx->state = STATE_ERROR;
        return false;
    }

    ctx->particles_allocated++;
    return true;
}

static bool detscat_parser_parse_line_prt(DetScatParser *parser,
                                          DetScatParserPrtContext *ctx) {
    assert(parser && ctx);

    char *trimmed = detscat_str_raw_trim(parser->current_line.data);
    if (trimmed[0] == '\0' || trimmed[0] == '#') return true;

    switch (ctx->state) {
        case STATE_INITIAL:
            ctx->state = STATE_WAIT_SECTION;
            // fall through

        case STATE_WAIT_SECTION:
            if (!detscat_parser_wait_prt(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case STATE_PARSE_TYPES_META:
            if (!detscat_parser_types_meta_prt(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case STATE_PARSE_TYPES_DEF:
            if (!detscat_parser_types_def_prt(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case STATE_PARSE_PARTICLES_META:
            if (!detscat_parser_particles_meta_prt(trimmed, ctx, parser))
                goto error_cleanup;
            return true;
        case STATE_PARSE_PARTICLES_DEF:
            if (!detscat_parser_particles_prt(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case STATE_ERROR:
            goto error_cleanup;

        default:
            return true;
    }

error_cleanup:
    detscat_prt_free_subset(ctx->prt, ctx->types_allocated,
                            ctx->particles_allocated);
    return false;
}

static bool detscat_parser_initial_par(const char *line,
                                       DetScatParserParContext *ctx,
                                       DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Received an empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }



    if (strstr(trimmed, "NCOMP")) {
        size_t n_components;
        if (sscanf(trimmed, "%zu", &n_components) != 1 ||
            n_components == 0) {
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Invalid NCOMP format at line %d",
                     parser->lineno);
            parser->status = DETSCAT_PARSER_ERR_FORMAT;
            goto error_cleanup;
        }

        par->n_components = n_components;
        par->components = calloc(n_components,
                                 sizeof(*par->components));
        if (!par->components) {
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Memory allocation for components failed");
            parser->status = DETSCAT_PARSER_ERR_ALLOC;
            goto error_cleanup;
        }

        ctx->state = PARSE_COMP;
    } else if (strstr(trimmed, "NPLANES")) {
        size_t n_scat_planes;
        if (sscanf(trimmed, "%zu", &n_scat_planes) != 1 ||
            n_scat_planes == 0) {
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Invalid NPLANES format at line %d",
                     parser->lineno);
            parser->status = DETSCAT_PARSER_ERR_FORMAT;
            goto error_cleanup;
        }

        par->n_scat_planes = n_scat_planes;
        par->scat_planes = calloc(n_scat_planes,
                sizeof(double[DETSCAT_DDSCAT_SCAT_PLANE_PARAMS]));
        if (!par->scat_planes) {
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Memory allocation for scattering planes "
                     "failed");
            parser->status = DETSCAT_PARSER_ERR_ALLOC;
            goto error_cleanup;
        }

        ctx->state = PARSE_PLANES;
    } else if (strstr(trimmed, "Polarization state")) {
        if (sscanf(trimmed, "(%lf, %lf) (%lf, %lf) (%lf, %lf)",
                   &par->e01.x.re, &par->e01.x.im, &par->e01.y.re,
                   &par->e01.y.im, &par->e01.z.re,
                   &par->e01.z.im) != 6) {
            snprintf(
                parser->errmsg, sizeof(parser->errmsg),
                "Invalid format for polarization state at line %d",
                parser->lineno);
            parser->status = DETSCAT_PARSER_ERR_FORMAT;
            goto error_cleanup;
        }
        ctx->state = PARSE_INITIAL;
    }
    return true;

}





static bool detscat_ddscat_parse_par_line(DetScatParser *parser,
                                          DetScatParserParContext *ctx) { 
    assert(parser && ctx);

    char *trimmed = detscat_str_raw_trim(parser->current_line.data);

    switch (ctx->state) {
        case PAR_INITIAL:
            if (!detscat_parser_initial_par(trimmed, ctx, parser))
                goto error_cleanup;

        case PARSE_COMP:
            if (ctx->components_allocated >= par->n_components) {
                snprintf(parser->errmsg, sizeof(parser->errmsg),
                         "Too many components provided (expected %zu) at "
                         "line %d",
                         par->n_components, parser->lineno);
                parser->status = DETSCAT_PARSER_ERR_FORMAT;
                goto error_cleanup;
            }

            if (!str_init(&par->components[ctx->components_allocated]) ||
                !str_reserve(&par->components[ctx->components_allocated],
                             DETSCAT_DDSCAT_PATH_INIT)) {
                parser->status = DETSCAT_PARSER_ERR_ALLOC;
                snprintf(parser->errmsg, sizeof(parser->errmsg),
                         "Memory allocation failed for component path "
                         "at line %d",
                         parser->lineno);
                goto error_cleanup;
            }

            if (!detscat_ddscat_parse_par_component(
                    parser, &par->components[ctx->components_allocated], trimmed)) {
                goto error_cleanup;
            }

            ctx->components_allocated++;
            if (ctx->components_allocated == par->n_components) {
                ctx->state = PARSE_INITIAL;
            }
            return true;

        case PARSE_PLANES:
            if (ctx->scat_planes_parsed >= par->n_scat_planes) {
                snprintf(parser->errmsg, sizeof(parser->errmsg),
                         "Too many scattering planes provided (expected "
                         "%zu) at line %d",
                         par->n_scat_planes, parser->lineno);
                parser->status = DETSCAT_PARSER_ERR_FORMAT;
                goto error_cleanup;
            }

            if (sscanf(trimmed, "%lf %lf %lf %lf",
                       &par->scat_planes[ctx->scat_planes_parsed][0],
                       &par->scat_planes[ctx->scat_planes_parsed][1],
                       &par->scat_planes[ctx->scat_planes_parsed][2],
                       &par->scat_planes[ctx->scat_planes_parsed][3]) !=
                DETSCAT_DDSCAT_SCAT_PLANE_PARAMS) {
                snprintf(parser->errmsg, sizeof(parser->errmsg),
                         "Invalid format for scattering plane at line %d",
                         parser->lineno);
                parser->status = DETSCAT_PARSER_ERR_FORMAT;
                goto error_cleanup;
            }

            ctx->scat_planes_parsed++;
            if (ctx->scat_planes_parsed == par->n_scat_planes) {
                ctx->state = PARSE_INITIAL;
            }
            return true;
    }

error_cleanup:
    detscat_ddscat_par_free_count(par, ctx->components_allocated);
    return false;
}




// --- Internal helpers (SHARED) ---
DetScatParser *detscat_parser_create(DetScatParserType type) {
    DetScatParser *parser = calloc(1, sizeof(*parser));
    if (!parser) return NULL;

    if (!detscat_str_init(&parser->current_line) ||
        !detscat_str_reserve(&parser->current_line, DETSCAT_PARSER_LINE_INIT)) {
        detscat_str_free(&parser->current_line);
        free(parser);
        return NULL;
    }

    assert(type >= 0 && type < DETSCAT_PARSER_TYPE_COUNT);
    parser->type = type;
    parser->status = DETSCAT_PARSER_OK;

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
    assert(parser && context);

    if (!file_path || !*file_path) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid file path for parser initialization");
        return false;
    }

    errno = 0;
    parser->stream = fopen(file_path, "r");
    if (!parser->stream) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FILE,
                         "Could not open '%s': %s", file_path, strerror(errno));
        return false;
    }

    assert(parser->type >= 0 && parser->type < DETSCAT_PARSER_TYPE_COUNT);
    uint32_t expected = expected_magic[parser->type];
    uint32_t actual = context ? *(uint32_t *)context : 0;

    if (actual != expected) {
        fclose(parser->stream);
        parser->stream = NULL;
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid context for %s",
                         detscat_parser_type_repr(parser->type));
        return false;
    }
    parser->context = context;

    return true;
}

bool detscat_parser_reset(DetScatParser *parser, const char *file_path,
                          void *context) {
    /* The reset function intentionally leaves the parser in an
     * invalid yet reusable state on failure. In such cases,
     * it is safe to either call reset again or destroy the parser. */
    assert(parser && context);

    if (!file_path || !*file_path) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid file path for resetting parser");
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
                         "Memory reset for parser line buffer failed");
        return false;
    }

    assert(parser->type >= 0 && parser->type < DETSCAT_PARSER_TYPE_COUNT);
    uint32_t expected = expected_magic[parser->type];
    uint32_t actual = context ? *(uint32_t *)context : 0;

    if (actual != expected) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid context for %s",
                         detscat_parser_type_repr(parser->type));
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
    assert(parser);

    if (!parser->stream) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_STREAM, "Invalid stream");
        return false;
    }

    if (!detscat_str_is_valid(&parser->current_line)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Line buffer is invalid/uninitialized");
        return false;
    }

    if (parser->type < 0 || parser->type >= DETSCAT_PARSER_TYPE_COUNT) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_TYPE,
                         "Invalid parser type '%s'",
                         detscat_parser_type_repr(parser->type));
        return false;
    }

    if (!parser->context ||
        expected_magic[parser->type] != *(uint32_t *)parser->context) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_CONTEXT, "Invalid context");
        return false;
    }

    if (parser->eof) {
        parser->status = DETSCAT_PARSER_EOF;
        return false;
    }

    // reset buffer
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
    assert(parser);

    assert(parser->stream);
    assert(parser->context);
    STR_ASSERT_VALID(&parser->current_line);

    assert(parser->type >= 0 && parser->type < DETSCAT_PARSER_TYPE_COUNT);

    switch (parser->type) {
        case DETSCAT_CFG: {
            assert(expected_magic[DETSCAT_CFG] == *(uint32_t *)parser->context);
            DetScatParserCfgContext *ctx =
                (DetScatParserCfgContext *)(parser->context);
            return detscat_parser_parse_line_cfg(parser, ctx);
        }

        case DETSCAT_PRT: {
            assert(expected_magic[DETSCAT_PRT] == *(uint32_t *)parser->context);
            DetScatParserPrtContext *ctx =
                (DetScatParserPrtContext *)(parser->context);
            return detscat_parser_parse_line_prt(parser, ctx);
        }
        default:
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_TYPE,
                             "Invalid parser type");
            return false;
    }
}

bool detscat_parser_check_final_state_prt(DetScatParser *parser,
                                          const char *file_path,
                                          void *context) {
    assert(parser && context);

    if (!file_path || !*file_path) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Function argument 'file_path' is invalid");
        return false;
    }

    assert(expected_magic[DETSCAT_PRT] == *(uint32_t *)parser->context);
    DetScatParserPrtContext *ctx = (DetScatParserPrtContext *)(parser->context);

    switch (ctx->state) {
        case STATE_PARSE_TYPES_DEF:
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Unexpected end of file: missing $(EndTypes)");
            break;

        case STATE_PARSE_PARTICLES_DEF:
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Unexpected end of file: missing $(EndParticles)");
            break;

        default: {
            const char* error_message;
            if (!ctx->types_parsed || !ctx->particles_parsed) {

                if (!ctx->types_parsed && !ctx->particles_parsed) {
                    error_message = "Missing sections: $(StartTypes) and $(StartParticles)";
                } else if (!ctx->types_parsed) {
                    error_message = "Missing section: $(StartTypes)";
                } else {
                    error_message = "Missing section: $(StartParticles)";
                }
                PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                                 "%s", error_message);
                break;
            }
            return true;
        }
    }

    detscat_prt_free_subset(ctx->prt, ctx->types_allocated, ctx->particles_allocated);

    return false;
}

const char *detscat_parser_type_repr(DetScatParserType type) {
    switch (type) {
#define X(name, str)     \
    case DETSCAT_##name: \
        return str;
        DETSCAT_PARSER_TYPE_LIST
#undef X
        default:
            return "UNKNOWN Type";
    }
}
