/* detscat_parser.c
 *
 * FUNCTIONS NAVIGATION:
 * ---------------------
 *
 * Internal helpers (PRIVATE)
 * --------------------------
 *
 *    // CFG
 *    - detscat_parser_parse_bool_cfg
 *    - detscat_parser_parse_double_cfg
 *    - detscat_parser_parse_int_cfg
 *    - detscat_parser_parse_vec3_cfg
 *    - detscat_parser_parse_cplx_vec3_cfg
 *    - detscat_parser_parse_cplx_cfg
 *    - detscat_parser_parse_str_cfg
 *    - detscat_parser_split_key_value_cfg
 *    - detscat_parser_handle_key_cfg
 *    - detscat_parser_parse_line_cfg
 *
 *    // PRT
 *    - detscat_parser_parse_double_prt
 *    - detscat_parser_parse_int_prt
 *    - detscat_parser_parse_type_prt
 *    - detscat_parser_parse_particle_prt
 *    - detscat_parser_wait_prt
 *    - detscat_parser_types_meta_prt
 *    - detscat_parser_types_def_prt
 *    - detscat_parser_particles_meta_prt
 *    - detscat_parser_particles_prt
 *    - detscat_parser_parse_line_prt
 *
 *    // PAR
 *    - detscat_parser_parse_component_par
 *    - detscat_parser_get_params_by_name_par
 *    - detscat_parser_parse_sampling_params_par
 *    - detscat_parser_initial_par
 *    - detscat_parser_parse_components_par
 *    - detscat_parser_parse_orientations_par
 *    - detscat_parser_parse_wavelengths_par
 *    - detscat_parser_parse_radii_par
 *    - detscat_parser_parse_scat_planes_par
 *    - detscat_parser_parse_line_par
 *
 *    // FML
 *    - detscat_parser_initial_fml
 *    - detscat_parser_find_header_fml
 *    - detscat_parser_alloc_fmats_fml
 *    - detscat_parser_alloc_scat_plane_fml
 *    - detscat_parser_read_values_fml
 *    - detscat_parser_next_scat_plane_fml
 *    - detscat_parser_parse_line_fml
 *
 *
 * Internal helpers (SHARED)
 * --------------------------
 *
 *    - detscat_parser_create
 *    - detscat_parser_destroy
 *    - detscat_parser_init
 *    - detscat_parser_next_line
 *    - detscat_parser_parse_line
 *    - detscat_parser_check_final_state_prt
 *    - detscat_parser_check_final_state_par
 *    - detscat_parser_type_repr
 *
 */
#include "detscat_parser.h"

#include "detscat_cfg.h"
#include "detscat_limits.h"
#include "detscat_math.h"
#include "detscat_prt.h"
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


// --- Expected parser context signatures  ---
static const uint32_t expected_magic[] = {[DETSCAT_CFG] = DETSCAT_CFG_MAGIC,
                                          [DETSCAT_PRT] = DETSCAT_PRT_MAGIC,
                                          [DETSCAT_PAR] = DETSCAT_PAR_MAGIC,
                                          [DETSCAT_FML] = DETSCAT_FML_MAGIC};

// --- Internal helpers (PRIVATE) ---
//------------------------------------------------------------------------------
// CFG Parser
//------------------------------------------------------------------------------
static bool detscat_parser_parse_bool_cfg(const char *value, bool *out,
                                          DetScatParser *parser,
                                          const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid or empty key/value provided at line %d",
                         parser->line_number);
        return false;
    }

    if (detscat_str_raw_strcasecmp(value, "true") == 0) {
        *out = true;
    } else if (detscat_str_raw_strcasecmp(value, "false") == 0) {
        *out = false;
    } else {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Invalid format for '%s' at line %d: expected a boolean", key,
            parser->line_number);
        return false;
    }

    return true;
}

static bool detscat_parser_parse_double_cfg(const char *value, double *out,
                                            DetScatParser *parser,
                                            const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid or empty key/value provided at line %d",
                         parser->line_number);
        return false;
    }

    errno = 0;
    char *endptr = NULL;
    double val = strtod(value, &endptr);

    if (errno == ERANGE) {
        if (val == HUGE_VAL || val == -HUGE_VAL) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_OVERFLOW,
                             "Overflow parsing '%s' at line %d", key,
                             parser->line_number);
            return false;
        } else {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_UNDERFLOW,
                             "Underflow parsing '%s' at line %d", key,
                             parser->line_number);
            return false;
        }
    }

    if (endptr == value || *endptr != '\0') {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Invalid format for '%s' at line %d: expected a double", key,
            parser->line_number);
        return false;
    }

    *out = val;

    return true;
}

static bool detscat_parser_parse_int_cfg(const char *value, int *out,
                                         DetScatParser *parser,
                                         const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid or empty key/value provided at line %d",
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
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Invalid format for '%s' at line %d: expected an integer", key,
            parser->line_number);
        return false;
    }

    // safety check for casting long to int
    if (val < INT_MIN || val > INT_MAX) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Integer value out of range for '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    *out = (int)val;

    return true;
}

static bool detscat_parser_parse_vec3_cfg(const char *value, Vec3 *out,
                                          DetScatParser *parser,
                                          const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid or empty key/value provided at line %d",
                         parser->line_number);

        return false;
    }

    int matched =
        sscanf(value, "[ %lf , %lf , %lf ]", &out->x, &out->y, &out->z);
    if (matched != 3) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid format for '%s' at line %d: expectec vec3",
                         key, parser->line_number);
        return false;
    }

    return true;
}

static bool detscat_parser_parse_cplx_vec3_cfg(const char *value,
                                               ComplexVec3 *out,
                                               DetScatParser *parser,
                                               const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid or empty key/value provided at line %d",
                         parser->line_number);

        return false;
    }

    int matched = sscanf(
        value, "[ [ %lf , %lf ] , [ %lf , %lf ] , [ %lf , %lf ] ]", &out->x.re,
        &out->x.im, &out->y.re, &out->y.im, &out->z.re, &out->z.im);
    if (matched != 6) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Invalid format for '%s' at line %d: expected a complex vec3", key,
            parser->line_number);
        return false;
    }

    return true;
}

static bool detscat_parser_parse_cplx_cfg(const char *value,
                                          Complex *out,
                                          DetScatParser *parser,
                                          const char *key) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid or empty key/value provided at line %d",
                         parser->line_number);

        return false;
    }

    int matched = sscanf(value, " [ %lf , %lf ] ", &out->re, &out->im);
    if (matched != 2) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Invalid format for '%s' at line %d: expected a complex number",
            key, parser->line_number);
        return false;
    }

    return true;
}

static bool detscat_parser_parse_str_cfg(const char *value, Str *out,
                                         DetScatParser *parser, const char *key,
                                         size_t maxlen) {
    assert(parser && out);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid or empty key/value provided at line %d",
                         parser->line_number);

        return false;
    }

    if (strlen(value) >= maxlen) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Path too long for '%s' at line %d (max %zu chars)",
                         key, parser->line_number, DETSCAT_CFG_PATH_MAX - 1);
        return false;
    }

    if (!detscat_str_set(out, value)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Could not assign string for '%s' at line %d", key,
                         parser->line_number);
        return false;
    }

    return true;
}

static bool detscat_parser_split_key_value_cfg(char *line, char **key,
                                               char **value) {
    assert(key && value);

    if (!line || !*line) return false;

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

static bool detscat_parser_handle_key_cfg(const char *key, const char *value,
                                          DetScatConfig *cfg,
                                          DetScatParser *parser) {
    assert(parser && cfg);

    if (!value || !*value || !key || !*key) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid or empty key/value provided at line %d",
                         parser->line_number);
        return false;
    }

    if (strcmp(key, "e01_coefficient") == 0) {
        return detscat_parser_parse_cplx_cfg(value, &cfg->e01_coeff, parser,
                                             key);
    } else if (strcmp(key, "e02_coefficient") == 0) {
        return detscat_parser_parse_cplx_cfg(value, &cfg->e02_coeff, parser,
                                             key);
    } else if (strcmp(key, "light_source_direction") == 0) {
        return detscat_parser_parse_vec3_cfg(
            value, &cfg->light_source_direction, parser, key);
    } else if (strcmp(key, "wavelength_nm") == 0) {
        return detscat_parser_parse_double_cfg(value, &cfg->wavelength_nm,
                                               parser, key);
    } else if (strcmp(key, "pulse_energy_mj") == 0) {
        return detscat_parser_parse_double_cfg(value, &cfg->pulse_energy_mj,
                                               parser, key);
    } else if (strcmp(key, "pulse_width_ns") == 0) {
        return detscat_parser_parse_double_cfg(value, &cfg->pulse_width_ns,
                                               parser, key);
    } else if (strcmp(key, "beam_diameter_mm") == 0) {
        return detscat_parser_parse_double_cfg(value, &cfg->beam_diameter_mm,
                                               parser, key);
    } else if (strcmp(key, "particles_definition_file") == 0) {
        return detscat_parser_parse_str_cfg(value, &cfg->particles_file_path,
                                            parser, key, DETSCAT_CFG_PATH_MAX);
    } else if (strcmp(key, "camera_center_position_m") == 0) {
        return detscat_parser_parse_vec3_cfg(
            value, &cfg->camera_center_position_m, parser, key);
    } else if (strcmp(key, "camera_direction") == 0) {
        return detscat_parser_parse_vec3_cfg(value, &cfg->camera_direction,
                                             parser, key);
    } else if (strcmp(key, "sensor_width_mm") == 0) {
        return detscat_parser_parse_double_cfg(value, &cfg->sensor_width_mm,
                                               parser, key);
    } else if (strcmp(key, "sensor_height_mm") == 0) {
        return detscat_parser_parse_double_cfg(value, &cfg->sensor_height_mm,
                                               parser, key);
    } else if (strcmp(key, "focal_length_mm") == 0) {
        return detscat_parser_parse_double_cfg(value, &cfg->focal_length_mm,
                                               parser, key);
    } else if (strcmp(key, "sensor_resolution_x_px") == 0) {
        return detscat_parser_parse_int_cfg(value, &cfg->sensor_resolution_x_px,
                                            parser, key);
    } else if (strcmp(key, "sensor_resolution_y_px") == 0) {
        return detscat_parser_parse_int_cfg(value, &cfg->sensor_resolution_y_px,
                                            parser, key);
    } else {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_UNKNOWN_KEY,
                         "Unknown key '%s' at line %d", key,
                         parser->line_number);
        return false;
    }
}

static bool detscat_parser_parse_line_cfg(DetScatParserCfgContext *ctx,
                                          DetScatParser *parser) {
    assert(parser && ctx);

    char *trimmed = detscat_str_raw_trim(parser->current_line.data);
    if (trimmed[0] == '\0' || trimmed[0] == '#') return true;

    char *key = NULL, *value = NULL;
    if (!detscat_parser_split_key_value_cfg(trimmed, &key, &value)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_SYNTAX,
                         "Could not obtain key-value pair at line %d",
                         parser->line_number);
        return false;
    }

    if (!detscat_parser_handle_key_cfg(key, value, ctx->cfg, parser))
        return false;

    return true;
}

//------------------------------------------------------------------------------
// PRT Parser
//------------------------------------------------------------------------------
static bool detscat_parser_parse_double_prt(const char *str, double *out,
                                            DetScatParser *parser) {
    assert(parser && out);

    if (!str || !*str) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid or empty value string provided at line %d",
                         parser->line_number);
        return false;
    }

    char *endptr;
    errno = 0;
    double val = strtod(str, &endptr);

    if (errno == ERANGE) {
        if (val == HUGE_VAL || val == -HUGE_VAL) {
            PARSER_SET_ERROR(
                parser, DETSCAT_PARSER_ERR_OVERFLOW,
                "Numeric value at line %d is out of range (overflow)",
                parser->line_number);
            return false;
        } else {
            PARSER_SET_ERROR(
                parser, DETSCAT_PARSER_ERR_UNDERFLOW,
                "Numeric value at line %d is too small (underflow)",
                parser->line_number);
            return false;
        }
    }

    if (endptr == str || *endptr != '\0') {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid format at line %d: expected a double",
                         parser->line_number);
        return false;
    }

    *out = val;

    return true;
}

static bool detscat_parser_parse_int_prt(const char *str, int *out,
                                         DetScatParser *parser) {
    assert(parser && out);

    if (!str || !*str) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid or empty value string provided at line %d",
                         parser->line_number);
        return false;
    }

    errno = 0;
    char *endptr = NULL;
    long val = strtol(str, &endptr, 10);

    if ((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Overflow/underflow parsing at line %d",
                         parser->line_number);
        return false;
    }

    if (endptr == str || *endptr != '\0') {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid format at line %d: expected an integer",
                         parser->line_number);
        return false;
    }

    if (val > INT_MAX || val < INT_MIN) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Integer value out of range for at line %d",
                         parser->line_number);
        return false;
    }

    *out = (int)val;

    return true;
}

static bool detscat_parser_parse_type_prt(const char *line,
                                          DetScatPrtType *type,
                                          DetScatParser *parser) {
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
            "Could not allocate temporary memory for buffer copy at line %d",
            parser->line_number);
        return false;
    }

    char *token1 = strtok(copy, " \t");
    char *token2 = strtok(NULL, " \t");
    char *token3 = strtok(NULL, " \t");

    if (!token1 || !token2 || token3) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_SYNTAX,
                         "Invalid particle type definition at line %d",
                         parser->line_number);
        goto error_cleanup;
    }

    if (strlen(token1) >= DETSCAT_PRT_TYPEID_MAX ||
        strlen(token2) >= DETSCAT_PRT_PATH_MAX) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Particle type ID or data path too long at line %d",
                         parser->line_number);
        goto error_cleanup;
    }

    if (!detscat_str_set(&type->type_id, token1) ||
        !detscat_str_set(&type->data_dir, token2)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not set particle type data at line %d",
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

static bool detscat_parser_parse_particle_prt(const char *line,
                                              DetScatPrtParticle *particle,
                                              DetScatParser *parser) {
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
            "Could not allocate temporary memory for buffer copy at line %d",
            parser->line_number);
        return false;
    }

    double a1[3];
    double a2[3];
    double xyz[3];

    // type id
    char *tok = strtok(copy, " \t");

    if (!tok) {
        free(copy);
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_SYNTAX,
                         "Invalid particle definition at line %d",
                         parser->line_number);
        return false;
    }

    if (strlen(tok) >= DETSCAT_PRT_TYPEID_MAX) {
        free(copy);
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Particle type ID too long at line %d",
                         parser->line_number);
        return false;
    }

    char *type_id = detscat_str_raw_strdup(tok);


    // wavelength
    tok = strtok(NULL, " \t");
    if (!tok) goto error_cleanup;
    if (!detscat_parser_parse_double_prt(tok, &particle->wavelength_nm, parser))
        goto error_cleanup;

    // eff_raidus
    tok = strtok(NULL, " \t");
    if (!tok) goto error_cleanup;
    if (!detscat_parser_parse_double_prt(tok, &particle->eff_radius_um, parser))
        goto error_cleanup;

    // a1
    for (size_t i = 0; i < 3; ++i) {
        tok = strtok(NULL, " \t");
        if (!tok) goto error_cleanup;
        if (!detscat_parser_parse_double_prt(tok, &a1[i], parser))
            goto error_cleanup;
    }

    // a2
    for (size_t i = 0; i < 3; ++i) {
        tok = strtok(NULL, " \t");
        if (!tok) goto error_cleanup;
        if (!detscat_parser_parse_double_prt(tok, &a2[i], parser))
            goto error_cleanup;
    }

    // position - xyz
    for (size_t i = 0; i < 3; ++i) {
        tok = strtok(NULL, " \t");
        if (!tok) goto error_cleanup;
        if (!detscat_parser_parse_double_prt(tok, &xyz[i], parser))
            goto error_cleanup;
    }

    tok = strtok(NULL, " \t");
    if (tok) goto error_cleanup;

    if (!detscat_str_set(&particle->type_id, type_id)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not set particle type ID at line %d",
                         parser->line_number);
        goto error_cleanup;
    }

    particle->orientation.a1.x = a1[0];
    particle->orientation.a1.y = a1[1];
    particle->orientation.a1.z = a1[2];

    particle->orientation.a2.x = a2[0];
    particle->orientation.a2.y = a2[1];
    particle->orientation.a2.z = a2[2];

    particle->position.x = xyz[0];
    particle->position.y = xyz[1];
    particle->position.z = xyz[2];

    free(copy);
    free(type_id);
    return true;

error_cleanup:
    PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_SYNTAX,
                     "Invalid particle definition at line %d",
                     parser->line_number);
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
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (strcmp(line, "$(StartTypes)") == 0) {
        if (ctx->types_parsed) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Duplicate $(StartTypes) at line %d",
                             parser->line_number);
            ctx->state = PRT_STATE_ERROR;
            return false;
        }
        ctx->state = PRT_STATE_PARSE_TYPES_META;
        return true;
    } else if (strcmp(line, "$(StartParticles)") == 0) {
        if (ctx->particles_parsed) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Duplicate $(StartParticles) at line %d",
                             parser->line_number);
            ctx->state = PRT_STATE_ERROR;
            return false;
        }
        ctx->state = PRT_STATE_PARSE_PARTICLES_META;
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
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (sscanf(line, " %zu ", &ctx->prt->n_types) != 1 ||
        ctx->prt->n_types == 0) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_SYNTAX,
            "Invalid or missing number of particle types at line %d",
            parser->line_number);
        ctx->state = PRT_STATE_ERROR;
        return false;
    }

    ctx->prt->types = calloc(ctx->prt->n_types, sizeof(*(ctx->prt->types)));
    if (!ctx->prt->types) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not allocate memory for particle types");
        ctx->state = PRT_STATE_ERROR;
        return false;
    }

    ctx->state = PRT_STATE_PARSE_TYPES_DEF;
    return true;
}

static bool detscat_parser_types_def_prt(const char *line,
                                         DetScatParserPrtContext *ctx,
                                         DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (strcmp(line, "$(StartParticles)") == 0) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Missing $(EndTypes) before $(StartParticles) at line %d",
            parser->line_number);
        ctx->state = PRT_STATE_ERROR;
        return false;
    }

    if (strcmp(line, "$(EndTypes)") == 0) {
        if (ctx->types_allocated != ctx->prt->n_types) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Expected %zu particle type definitions, got %zu",
                             ctx->prt->n_types, ctx->types_allocated);
            ctx->state = PRT_STATE_ERROR;
            return false;
        }
        ctx->types_parsed = true;
        ctx->state = PRT_STATE_WAIT_SECTION;
        return true;
    }

    if (ctx->types_allocated >= ctx->prt->n_types) {
        parser->status = DETSCAT_PARSER_ERR_FORMAT;
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Too many particle type definitions. Stopped at line %d",
            parser->line_number);
        ctx->state = PRT_STATE_ERROR;
        return false;
    }

    if (!detscat_str_init(&ctx->prt->types[ctx->types_allocated].type_id) ||
        !detscat_str_init(&ctx->prt->types[ctx->types_allocated].data_dir) ||
        !detscat_str_reserve(&ctx->prt->types[ctx->types_allocated].type_id,
                             DETSCAT_PRT_TYPEID_INIT) ||
        !detscat_str_reserve(&ctx->prt->types[ctx->types_allocated].data_dir,
                             DETSCAT_PRT_PATH_INIT)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not allocate memory for particle type ID or "
                         "data path at line %d",
                         parser->line_number);
        ctx->state = PRT_STATE_ERROR;
        return false;
    }

    if (!detscat_parser_parse_type_prt(
            line, &ctx->prt->types[ctx->types_allocated], parser)) {
        ctx->state = PRT_STATE_ERROR;
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
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (sscanf(line, "%zu", &ctx->prt->n_particles) != 1 ||
        ctx->prt->n_particles == 0) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid or missing number of particles at line %d",
                         parser->line_number);
        ctx->state = PRT_STATE_ERROR;
        return false;
    }
    ctx->prt->particles =
        calloc(ctx->prt->n_particles, sizeof(*(ctx->prt->particles)));
    if (!ctx->prt->particles) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not allocate memory for particles");
        ctx->state = PRT_STATE_ERROR;
        return false;
    }
    ctx->state = PRT_STATE_PARSE_PARTICLES_DEF;
    return true;
}

static bool detscat_parser_particles_prt(const char *line,
                                         DetScatParserPrtContext *ctx,
                                         DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (strcmp(line, "$(StartTypes)") == 0) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Missing $(EndParticles) before $(StartTypes) at line %d",
            parser->line_number);
        ctx->state = PRT_STATE_ERROR;
        return false;
    }

    if (strcmp(line, "$(EndParticles)") == 0) {
        if (ctx->particles_allocated != ctx->prt->n_particles) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Expected %zu particles, got %zu",
                             ctx->prt->n_particles, ctx->particles_allocated);
            ctx->state = PRT_STATE_ERROR;
            return false;
        }
        ctx->particles_parsed = true;
        ctx->state = PRT_STATE_WAIT_SECTION;
        return true;
    }

    if (ctx->particles_allocated >= ctx->prt->n_particles) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Too many particles defined. Expected %zu, got %zu. "
                         "Stopped at line %d",
                         ctx->prt->n_particles, ctx->particles_allocated,
                         parser->line_number);
        ctx->state = PRT_STATE_ERROR;
        return false;
    }

    if (!detscat_str_init(
            &ctx->prt->particles[ctx->particles_allocated].type_id) ||
        !detscat_str_reserve(
            &ctx->prt->particles[ctx->particles_allocated].type_id,
            DETSCAT_PRT_TYPEID_INIT)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not allocate memory for particle type ID "
                         "at line %d",
                         parser->line_number);
        ctx->state = PRT_STATE_ERROR;
        return false;
    }

    if (!detscat_parser_parse_particle_prt(
            line, &ctx->prt->particles[ctx->particles_allocated], parser)) {
        ctx->state = PRT_STATE_ERROR;
        return false;
    }

    ctx->particles_allocated++;
    return true;
}

static bool detscat_parser_parse_line_prt(DetScatParserPrtContext *ctx,
                                          DetScatParser *parser) {
    assert(parser && ctx);

    char *trimmed = detscat_str_raw_trim(parser->current_line.data);
    if (trimmed[0] == '\0' || trimmed[0] == '#') return true;

    switch (ctx->state) {
        case PRT_STATE_INITIAL:
            ctx->state = PRT_STATE_WAIT_SECTION;
            // fall through

        case PRT_STATE_WAIT_SECTION:
            if (!detscat_parser_wait_prt(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case PRT_STATE_PARSE_TYPES_META:
            if (!detscat_parser_types_meta_prt(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case PRT_STATE_PARSE_TYPES_DEF:
            if (!detscat_parser_types_def_prt(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case PRT_STATE_PARSE_PARTICLES_META:
            if (!detscat_parser_particles_meta_prt(trimmed, ctx, parser))
                goto error_cleanup;
            return true;
        case PRT_STATE_PARSE_PARTICLES_DEF:
            if (!detscat_parser_particles_prt(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case PRT_STATE_ERROR:
            goto error_cleanup;

        default:
            return true;
    }

error_cleanup:
    detscat_prt_free_subset(ctx->prt, ctx->types_allocated,
                            ctx->particles_allocated);
    return false;
}

//------------------------------------------------------------------------------
// PAR Parser
//------------------------------------------------------------------------------
static bool detscat_parser_parse_component_par(const char *line, Str *component,
                                               DetScatParser *parser) {
    assert(parser && component);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    const char *start = strchr(line, '\'');
    if (!start) goto cleanup;
    start++;

    const char *end = strchr(start, '\'');
    if (!end) goto cleanup;

    size_t len = end - start;

    if (len >= DETSCAT_DDSCAT_PATH_MAX) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_RANGE,
                         "Component data path too long at line %d",
                         parser->line_number);
        return false;
    }

    char *path = malloc(len + 1);
    if (!path) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_MEMORY,
            "Could not allocate temporary memory for data path at line %d",
            parser->line_number);
        return false;
    }
    memcpy(path, start, len);
    path[len] = '\0';

    if (!detscat_str_set(component, path)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not set component data path at line %d",
                         parser->line_number);
        free(path);
        return false;
    }
    detscat_str_raw_normpath(component->data);

    free(path);
    return true;

cleanup:
    PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                     "Invalid component data path at line %d",
                     parser->line_number);
    return false;
}



static DetScatDdscatSamplingParams *detscat_parser_get_params_by_name_par(
    const char *name, DetScatParserParContext *ctx) 
{
    if (strcmp(name, "BETA") == 0) return &ctx->beta_params;
    if (strcmp(name, "THETA") == 0) return &ctx->theta_params;
    if (strcmp(name, "PHI") == 0) return &ctx->phi_params;
    if (strcmp(name, "RADIUS") == 0) return &ctx->radius_params;
    if (strcmp(name, "WAVELENGTH") == 0) return &ctx->wavelength_params;
    return NULL;
}

static bool detscat_parser_parse_sampling_params_par(
    const char *line, const char *param_name, DetScatParserParContext *ctx,
    DetScatParser *parser, bool parse_method) {
    assert(parser && ctx && param_name);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }


    DetScatDdscatSamplingParams *params; 
    params = detscat_parser_get_params_by_name_par(param_name, ctx);
    if (!params) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid parameter name '%s'", param_name);
        return false;
    }

    int match;
    if (parse_method) {
        match = sscanf(line, " %lf %lf %zu '%8s' ",
                       &params->min,
                       &params->max,
                       &params->n,
                       params->method);
    } else {
        match = sscanf(line, " %lf %lf %zu ",
                       &params->min,
                       &params->max,
                       &params->n);
    }

    if (match != (parse_method ? 4 : 3)) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid format for %s at line %d",
                         param_name,
                         parser->line_number);
        return false;
    }

    return true;
}

static bool detscat_parser_initial_par(const char *line,
                                       DetScatParserParContext *ctx,
                                       DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (strstr(line, "NCOMP")) {
        size_t n_components;
        if (sscanf(line, "%zu", &n_components) != 1 || n_components == 0) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Invalid NCOMP format at line %d",
                             parser->line_number);
            return false;
        }

        ctx->par->n_components = n_components;
        ctx->par->components =
            calloc(n_components, sizeof(*ctx->par->components));
        if (!ctx->par->components) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                             "Could not allocate memory for components");
            return false;
        }

        ctx->state = PAR_STATE_PARSE_COMPONENTS;
    } else if (strstr(line, "NPLANES")) {
        size_t n_scat_planes;
        if (sscanf(line, "%zu", &n_scat_planes) != 1 || n_scat_planes == 0) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Invalid NPLANES format at line %d",
                             parser->line_number);
            return false;
        }

        ctx->par->n_scat_planes = n_scat_planes;
        ctx->par->scat_planes =
            calloc(n_scat_planes, sizeof(*ctx->par->scat_planes));
        if (!ctx->par->scat_planes) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                             "Could not allocate memory for scattering planes");
            return false;
        }

        ctx->state = PAR_STATE_PARSE_SCAT_PLANES;
    } else if (strstr(line, "NBETA")) {    
        if (!detscat_parser_parse_sampling_params_par(
            line, "BETA", ctx, parser, false))
            return false;

        ctx->angles_parsed++;
        ctx->state = (ctx->angles_parsed == 3)
                        ? PAR_STATE_PARSE_ORIENTATIONS
                        : PAR_STATE_INITIAL;
    } else if (strstr(line, "NTHETA")) {
        if (!detscat_parser_parse_sampling_params_par(
            line, "THETA", ctx, parser, false))
            return false;

        ctx->angles_parsed++;
        ctx->state = (ctx->angles_parsed == 3) 
                        ? PAR_STATE_PARSE_ORIENTATIONS
                        : PAR_STATE_INITIAL;
    } else if (strstr(line, "NPHI")) {
        if (!detscat_parser_parse_sampling_params_par(
            line, "PHI", ctx, parser, false))
            return false;

        ctx->angles_parsed++;
        ctx->state = (ctx->angles_parsed == 3)
                        ? PAR_STATE_PARSE_ORIENTATIONS
                        : PAR_STATE_INITIAL;
    } else if (strstr(line, "Polarization state")) {
        if (sscanf(line, "(%lf, %lf) (%lf, %lf) (%lf, %lf)",
                   &ctx->par->e01.x.re, &ctx->par->e01.x.im,
                   &ctx->par->e01.y.re, &ctx->par->e01.y.im,
                   &ctx->par->e01.z.re, &ctx->par->e01.z.im) != 6) {
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Invalid format for polarization state at line %d",
                             parser->line_number);
            return false;
        }
        ctx->state = PAR_STATE_INITIAL;
    } else if (strstr(line, "wavelengths")) {
        if (!detscat_parser_parse_sampling_params_par(
            line, "WAVELENGTH", ctx, parser, false)) return false;
        ctx->state = PAR_STATE_PARSE_WAVELENGTHS;
    } else if (strstr(line, "radii")) {
        if (!detscat_parser_parse_sampling_params_par(
            line, "RADIUS", ctx, parser, false)) return false;
        ctx->state = PAR_STATE_PARSE_RADII;
    }
    return true;
}

static bool detscat_parser_parse_components_par(const char *line,
                                                DetScatParserParContext *ctx,
                                                DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (ctx->components_allocated >= ctx->par->n_components) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Too many components provided (expected %zu) at "
                         "line %d",
                         ctx->par->n_components, parser->line_number);
        return false;
    }

    if (!detscat_str_init(&ctx->par->components[ctx->components_allocated]) ||
        !detscat_str_reserve(&ctx->par->components[ctx->components_allocated],
                             DETSCAT_DDSCAT_PATH_INIT)) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_MEMORY,
            "Could not allocate memory for component data path at line %d",
            parser->line_number);
        return false;
    }

    if (!detscat_parser_parse_component_par(
            line, &ctx->par->components[ctx->components_allocated], parser)) {
        return false;
    }

    ctx->components_allocated++;
    if (ctx->components_allocated == ctx->par->n_components) {
        ctx->state = PAR_STATE_INITIAL;
    }
    return true;
}


static bool detscat_parser_parse_orientations_par(const char *line,
                                                  DetScatParserParContext *ctx,
                                                  DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    ctx->par->n_orientations = ctx->beta_params.n *
                               ctx->theta_params.n *
                               ctx->phi_params.n;

    ctx->par->orientations = calloc(
        ctx->par->n_orientations, sizeof(*ctx->par->orientations));
    if (!ctx->par->orientations) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_MEMORY,
            "Failed to allocate memory for orientations at line %d",
            parser->line_number);
        return false;
    }

    double *beta = calloc(ctx->beta_params.n, sizeof(double));
    double *phi= calloc(ctx->phi_params.n, sizeof(double));
    double *theta= calloc(ctx->theta_params.n, sizeof(double));
    if (!beta || !phi || !theta) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_MEMORY,
            "Could not allocate temporary memory for angles at line %d",
            parser->line_number);
        return false;
    }

    for (size_t i = 0; i < ctx->beta_params.n; i++) {
        double frac = (i + 0.5) / ctx->beta_params.n;
        beta[i] = ctx->beta_params.min +
                  frac * (ctx->beta_params.max - ctx->beta_params.min);
    }

    for (size_t i = 0; i < ctx->phi_params.n; i++) {
        double frac = (i + 0.5) / ctx->phi_params.n;
        phi[i] = ctx->phi_params.min +
                 frac * (ctx->phi_params.max - ctx->phi_params.min);
    }

    double cos_thetmi = cos(ctx->theta_params.min * M_PI / 180.0);
    double cos_thetmx = cos(ctx->theta_params.max * M_PI / 180.0);

    if (ctx->theta_params.n % 2 == 1) {
        // odd
        for (size_t i = 0; i < ctx->theta_params.n; i++) {
            double frac = (double)i / (ctx->theta_params.max - 1);
            double cos_theta = cos_thetmi + frac * (cos_thetmx - cos_thetmi);
            theta[i] = acos(cos_theta) * 180.0 / M_PI;
        }
    } else {
        // even 
        for (size_t i = 0; i < ctx->theta_params.n; i++) {
            double frac = (i + 0.5) / ctx->theta_params.n;
            double cos_theta = cos_thetmi + frac * (cos_thetmx - cos_thetmi);
            theta[i] = acos(cos_theta) * 180.0 / M_PI;
        }
    }

    size_t c = 0;
    for (size_t i = 0; i < ctx->theta_params.n; ++i) {
        for (size_t j = 0; j < ctx->beta_params.n; ++j) {
            for (size_t k = 0; k < ctx->phi_params.n; ++k) {
                ctx->par->orientations[c].theta = theta[i];
                ctx->par->orientations[c].beta = beta[j];
                ctx->par->orientations[c].phi = phi[k];
                c++;
            }
        }
    }

    free(beta); free(theta); free(phi);

    ctx->state = PAR_STATE_INITIAL;
    return true;
}

static bool detscat_parser_parse_wavelengths_par(const char *line,
                                                 DetScatParserParContext *ctx,
                                                 DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }


    ctx->par->n_wavelengths = ctx->wavelength_params.n;

    ctx->par->wavelengths = calloc(ctx->par->n_wavelengths,
                                   sizeof(*ctx->par->wavelengths));
    if (!ctx->par->wavelengths) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_MEMORY,
            "Could not allocate memory for wavelengths at line %d",
            parser->line_number);
        return false;
    }

    const char *method = ctx->wavelength_params.method;
    if (strcmp(method, "LIN")) {
        double step = 
            (ctx->wavelength_params.max - ctx->wavelength_params.min) /
            (ctx->wavelength_params.n - 1);
        for (size_t i = 0; i < ctx->wavelength_params.n; ++i) {
            ctx->par->wavelengths[i] = ctx->wavelength_params.min + i * step;
        }
    } else if (strcmp(method, "INV")) {
        double inv_start = 1.0 / ctx->wavelength_params.min;
        double inv_end   = 1.0 / ctx->wavelength_params.max;
        double step = (inv_end - inv_start) / (ctx->wavelength_params.n - 1);
        for (size_t i = 0; i < ctx->wavelength_params.n; ++i) {
            double inv_val = inv_start + i * step;
            ctx->par->wavelengths[i] = 1.0 / inv_val;
        }
    } else if (strcmp(method, "LOG")) {
        double log_start = log10(ctx->wavelength_params.min);
        double log_end   = log10(ctx->wavelength_params.max);
        double step = (log_end - log_start) / (ctx->wavelength_params.n - 1);
        for (size_t i = 0; i < ctx->wavelength_params.n; i++) {
            ctx->par->wavelengths[i] = pow(10.0, log_start + i * step);
        }
    } else {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_INVALID_ARG,
            "Invalid/unsupported sampling method '%s' provided on line %d",
            method, parser->line_number);
        return false;
    }

    ctx->state = PAR_STATE_INITIAL;
    return true;
}

static bool detscat_parser_parse_radii_par(const char *line,
                                           DetScatParserParContext *ctx,
                                           DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }


    ctx->par->n_radii= ctx->radius_params.n;

    ctx->par->radii = calloc(ctx->par->n_radii, sizeof(*ctx->par->radii));
    if (!ctx->par->radii) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_MEMORY,
            "Could not allocate memory for radii at line %d",
            parser->line_number);
        return false;
    }

    const char *method = ctx->radius_params.method;
    if (strcmp(method, "LIN")) {
        double step = 
            (ctx->radius_params.max - ctx->radius_params.min) /
            (ctx->radius_params.n - 1);
        for (size_t i = 0; i < ctx->radius_params.n; ++i) {
            ctx->par->radii[i] = ctx->radius_params.min + i * step;
        }
    } else if (strcmp(method, "INV")) {
        double inv_start = 1.0 / ctx->radius_params.min;
        double inv_end   = 1.0 / ctx->radius_params.max;
        double step = (inv_end - inv_start) / (ctx->radius_params.n - 1);
        for (size_t i = 0; i < ctx->radius_params.n; ++i) {
            double inv_val = inv_start + i * step;
            ctx->par->radii[i] = 1.0 / inv_val;
        }
    } else if (strcmp(method, "LOG")) {
        double log_start = log10(ctx->radius_params.min);
        double log_end   = log10(ctx->radius_params.max);
        double step = (log_end - log_start) / (ctx->radius_params.n - 1);
        for (size_t i = 0; i < ctx->radius_params.n; i++) {
            ctx->par->radii[i] = pow(10.0, log_start + i * step);
        }
    } else {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_INVALID_ARG,
            "Invalid/unsupported sampling method '%s' provided on line %d",
            method, parser->line_number);
        return false;
    }

    ctx->state = PAR_STATE_INITIAL;
    return true;
}

static bool detscat_parser_parse_scat_planes_par(const char *line,
                                             DetScatParserParContext *ctx,
                                             DetScatParser *parser) {
    assert(parser && ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (ctx->scat_planes_parsed >= ctx->par->n_scat_planes) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_FORMAT,
            "Too many scattering planes provided (expected %zu) at line %d",
            ctx->par->n_scat_planes, parser->line_number);
        return false;
    }

    if (sscanf(line, "%lf %lf %lf %lf",
               &ctx->par->scat_planes[ctx->scat_planes_parsed][0],
               &ctx->par->scat_planes[ctx->scat_planes_parsed][1],
               &ctx->par->scat_planes[ctx->scat_planes_parsed][2],
               &ctx->par->scat_planes[ctx->scat_planes_parsed][3]) != 4) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid format for scattering plane at line %d",
                         parser->line_number);
        return false;
    }

    ctx->scat_planes_parsed++;
    if (ctx->scat_planes_parsed == ctx->par->n_scat_planes) {
        ctx->state = PAR_STATE_INITIAL;
    }
    return true;
}

static bool detscat_parser_parse_line_par(DetScatParserParContext *ctx,
                                          DetScatParser *parser) {
    assert(parser && ctx);

    char *trimmed = detscat_str_raw_trim(parser->current_line.data);

    switch (ctx->state) {
        case PAR_STATE_INITIAL:
            if (!detscat_parser_initial_par(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case PAR_STATE_PARSE_COMPONENTS:
            if (!detscat_parser_parse_components_par(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case PAR_STATE_PARSE_SCAT_PLANES:
            if (!detscat_parser_parse_scat_planes_par(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case PAR_STATE_PARSE_ORIENTATIONS:
            if (!detscat_parser_parse_orientations_par(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case PAR_STATE_PARSE_WAVELENGTHS:
            if (!detscat_parser_parse_wavelengths_par(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case PAR_STATE_PARSE_RADII:
            if (!detscat_parser_parse_radii_par(trimmed, ctx, parser))
                goto error_cleanup;
            return true;
    }

error_cleanup:
    detscat_ddscat_par_free_subset(ctx->par, ctx->components_allocated);
    return false;
}

//------------------------------------------------------------------------------
// FML Parser
//------------------------------------------------------------------------------
static bool detscat_parser_initial_fml(DetScatParserFmlContext *ctx,
                                       DetScatParser *parser) {
    assert(parser && ctx);

    ctx->fml->n_fmats = ctx->par->n_scat_planes;
    if (ctx->fml->n_fmats == 0) {
        PARSER_SET_ERROR(
            parser, DETSCAT_PARSER_ERR_RANGE,
            "Number of scattering planes must be larger than zero");
        return false;
    }
    ctx->state = FML_STATE_FIND_HEADER;
    return true;
}

static bool detscat_parser_find_header_fml(const char *line,
                                           DetScatParserFmlContext *ctx,
                                           DetScatParser *parser) {
    assert(ctx);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    if (strstr(line, "Re")) {
        ctx->data_header_found = true;
        ctx->state = FML_STATE_ALLOC_FMATS;
    }
    return true;
}

static bool detscat_parser_alloc_fmats_fml(DetScatParserFmlContext *ctx,
                                           DetScatParser *parser) {
    assert(ctx && parser);

    ctx->fml->fmats = calloc(ctx->fml->n_fmats, sizeof(*ctx->fml->fmats));
    if (!ctx->fml->fmats) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not allocate memory for fmats");
        return false;
    }

    ctx->fml->refcount = 1;
    ctx->current_plane = 0;
    ctx->current_theta = 0;
    ctx->state = FML_STATE_ALLOC_SCAT_PLANE;
    return true;
}

static bool detscat_parser_alloc_scat_plane_fml(DetScatParserFmlContext *ctx,
                                                DetScatParser *parser) {
    assert(ctx && parser);

    size_t i = ctx->current_plane;

    double range = ctx->par->scat_planes[i][2] - ctx->par->scat_planes[i][1];
    double step = ctx->par->scat_planes[i][3];

    size_t n_theta;

    if (step <= 0.0 || range < 0.0) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Invalid step/range for plane %zu", i + 1);
        return false;
    }
    n_theta = (size_t)(range / step) + 1;

    ctx->fml->fmats[i].n_theta = n_theta;
    ctx->fml->fmats[i].phi = ctx->par->scat_planes[i][0];
    ctx->fml->fmats[i].theta = calloc(n_theta, sizeof(double));
    ctx->fml->fmats[i].f11 = calloc(n_theta, sizeof(Complex));
    ctx->fml->fmats[i].f12 = calloc(n_theta, sizeof(Complex));
    ctx->fml->fmats[i].f21 = calloc(n_theta, sizeof(Complex));
    ctx->fml->fmats[i].f22 = calloc(n_theta, sizeof(Complex));

    if (!ctx->fml->fmats[i].theta || !ctx->fml->fmats[i].f11 ||
        !ctx->fml->fmats[i].f12 || !ctx->fml->fmats[i].f21 ||
        !ctx->fml->fmats[i].f22) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_MEMORY,
                         "Could not allocate memory for f-matrix of scattering "
                         "plane %zu",
                         i + 1);
        return false;
    }

    ctx->current_theta = 0;
    ctx->state = FML_STATE_READ_VALUES;
    return true;
}

static bool detscat_parser_read_values_fml(const char *line,
                                           DetScatParserFmlContext *ctx,
                                           DetScatParser *parser) {
    assert(ctx && parser);

    if (!line || !*line) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Empty or null input buffer at line %d",
                         parser->line_number);
        return false;
    }

    size_t i = ctx->current_plane;
    size_t j = ctx->current_theta;

    if (sscanf(line, "%lf %*f %lf %lf %lf %lf %lf %lf %lf %lf",
               &ctx->fml->fmats[i].theta[j], &ctx->fml->fmats[i].f11[j].re,
               &ctx->fml->fmats[i].f11[j].im, &ctx->fml->fmats[i].f21[j].re,
               &ctx->fml->fmats[i].f21[j].im, &ctx->fml->fmats[i].f12[j].re,
               &ctx->fml->fmats[i].f12[j].im, &ctx->fml->fmats[i].f22[j].re,
               &ctx->fml->fmats[i].f22[j].im) != 9) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Could not parse line %d", parser->line_number);
        return false;
    }

    if (++ctx->current_theta >= ctx->fml->fmats[i].n_theta) {
        ctx->matrices_allocated++;
        ctx->state = FML_STATE_NEXT_SCAT_PLANE;
    }

    return true;
}

static bool detscat_parser_next_scat_plane_fml(DetScatParserFmlContext *ctx) {
    assert(ctx);

    if (++ctx->current_plane < ctx->fml->n_fmats) {
        ctx->state = FML_STATE_ALLOC_SCAT_PLANE;
    } else {
        ctx->state = FML_STATE_DONE;
    }
    return true;
}

static bool detscat_parser_parse_line_fml(DetScatParserFmlContext *ctx,
                                          DetScatParser *parser) {
    assert(parser && ctx);

    char *trimmed = detscat_str_raw_trim(parser->current_line.data);
    if (trimmed[0] == '\0') return true;

    switch (ctx->state) {
        case FML_STATE_INITIAL:
            if (!detscat_parser_initial_fml(ctx, parser)) goto error_cleanup;
            // fall through

        case FML_STATE_FIND_HEADER:
            if (!detscat_parser_find_header_fml(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case FML_STATE_ALLOC_FMATS:
            if (!detscat_parser_alloc_fmats_fml(ctx, parser))
                goto error_cleanup;
            // fall through

        case FML_STATE_ALLOC_SCAT_PLANE:
            if (!detscat_parser_alloc_scat_plane_fml(ctx, parser))
                goto error_cleanup;
            // fall through

        case FML_STATE_READ_VALUES:
            if (!detscat_parser_read_values_fml(trimmed, ctx, parser))
                goto error_cleanup;
            return true;

        case FML_STATE_NEXT_SCAT_PLANE:
            detscat_parser_next_scat_plane_fml(ctx);
            return true;

        case FML_STATE_DONE:
            return true;
    }
    return false;  // should not reach

error_cleanup:
    detscat_ddscat_fml_free_subset(ctx->fml, ctx->matrices_allocated);
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
                         void *ctx) {
    assert(parser && ctx);

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
    uint32_t actual = ctx ? *(uint32_t *)ctx : 0;

    if (actual != expected) {
        fclose(parser->stream);
        parser->stream = NULL;
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Invalid context for %s",
                         detscat_parser_type_repr(parser->type));
        return false;
    }
    parser->context = ctx;

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
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_CONTEXT,
                         "Invalid parser context for %s",
                         detscat_parser_type_repr(parser->type));
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
            return detscat_parser_parse_line_cfg(ctx, parser);
        }

        case DETSCAT_PRT: {
            assert(expected_magic[DETSCAT_PRT] == *(uint32_t *)parser->context);
            DetScatParserPrtContext *ctx =
                (DetScatParserPrtContext *)(parser->context);
            return detscat_parser_parse_line_prt(ctx, parser);
        }

        case DETSCAT_PAR: {
            assert(expected_magic[DETSCAT_PAR] == *(uint32_t *)parser->context);
            DetScatParserParContext *ctx =
                (DetScatParserParContext *)(parser->context);
            return detscat_parser_parse_line_par(ctx, parser);
        }

        case DETSCAT_FML: {
            assert(expected_magic[DETSCAT_FML] == *(uint32_t *)parser->context);
            DetScatParserFmlContext *ctx =
                (DetScatParserFmlContext *)(parser->context);
            return detscat_parser_parse_line_fml(ctx, parser);
        }

        default:
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_TYPE,
                             "Invalid parser type");
            return false;
    }
}

bool detscat_parser_check_final_state_prt(DetScatParser *parser,
                                          const char *file_path, void *ctx) {
    assert(parser && ctx);

    if (!file_path || !*file_path) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Function argument 'file_path' is invalid");
        return false;
    }

    assert(expected_magic[DETSCAT_PRT] == *(uint32_t *)parser->context);
    DetScatParserPrtContext *ctxt =
        (DetScatParserPrtContext *)(parser->context);

    switch (ctxt->state) {
        case PRT_STATE_PARSE_TYPES_DEF:
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Unexpected end of file: missing $(EndTypes)");
            break;

        case PRT_STATE_PARSE_PARTICLES_DEF:
            PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                             "Unexpected end of file: missing $(EndParticles)");
            break;

        default: {
            const char *error_message;
            if (!ctxt->types_parsed || !ctxt->particles_parsed) {
                if (!ctxt->types_parsed && !ctxt->particles_parsed) {
                    error_message =
                        "Missing sections: $(StartTypes) and $(StartParticles)";
                } else if (!ctxt->types_parsed) {
                    error_message = "Missing section: $(StartTypes)";
                } else {
                    error_message = "Missing section: $(StartParticles)";
                }
                PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT, "%s",
                                 error_message);
                break;
            }
            return true;
        }
    }

    detscat_prt_free_subset(ctxt->prt, ctxt->types_allocated,
                            ctxt->particles_allocated);
    return false;
}

bool detscat_parser_check_final_state_par(DetScatParser *parser,
                                          const char *file_path, void *ctx) {
    assert(parser && ctx);

    if (!file_path || !*file_path) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_INVALID_ARG,
                         "Function argument 'file_path' is invalid");
        return false;
    }

    assert(expected_magic[DETSCAT_PAR] == *(uint32_t *)parser->context);
    DetScatParserParContext *ctxt =
        (DetScatParserParContext *)(parser->context);

    if (ctxt->components_allocated != ctxt->par->n_components) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Expected %zu components but got %zu",
                         ctxt->par->n_components, ctxt->components_allocated);
        detscat_ddscat_par_free_subset(ctxt->par, ctxt->components_allocated);
        return false;
    }

    if (ctxt->scat_planes_parsed != ctxt->par->n_scat_planes) {
        PARSER_SET_ERROR(parser, DETSCAT_PARSER_ERR_FORMAT,
                         "Expected %zu scattering planes but got %zu",
                         ctxt->par->n_scat_planes, ctxt->scat_planes_parsed);
        detscat_ddscat_par_free_subset(ctxt->par, ctxt->components_allocated);
        return false;
    }

    return true;
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
