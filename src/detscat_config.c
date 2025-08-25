#include "detscat_config.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "detscat_diag.h"
#include "detscat_log.h"
#include "detscat_parser.h"
#include "str.h"

static const size_t DETSCAT_CFG_PATH_INIT = 512;
static const size_t DETSCAT_CFG_PATH_MAX = 4096;

static bool parse_int(const char *value, int *out, DetScatParser *parser,
                      const char *key) {
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);
    assert(parser != NULL);
    assert(key != NULL && key[0] != '\0');

    errno = 0;
    char *endptr = NULL;
    long val = strtol(value, &endptr, 10);
    if (errno != 0) {
        parser->status = DETSCAT_PARSER_ERR_RANGE;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Overflow/underflow parsing '%s' at line %d", key,
                 parser->lineno);
        return false;
    }

    if (endptr == value || *endptr != '\0') {
        parser->status = DETSCAT_PARSER_ERR_FORMAT;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Invalid integer format for '%s' at line %d", key,
                 parser->lineno);
        return false;
    }

    // check if casting long to int is safe
    if (val < INT_MIN || val > INT_MAX) {
        parser->status = DETSCAT_PARSER_ERR_RANGE;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Integer value out of range for '%s' at line %d", key,
                 parser->lineno);
        return false;
    }

    *out = (int)val;
    return true;
}

static bool parse_double(const char *value, double *out, DetScatParser *parser,
                         const char *key) {
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);
    assert(parser != NULL);
    assert(key != NULL && key[0] != '\0');

    errno = 0;
    char *endptr = NULL;
    double val = strtod(value, &endptr);
    if (errno != 0) {
        parser->status = DETSCAT_PARSER_ERR_RANGE;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Overflow/underflow parsing '%s' at line %d", key,
                 parser->lineno);
        return false;
    }

    if (endptr == value || *endptr != '\0') {
        parser->status = DETSCAT_PARSER_ERR_FORMAT;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Invalid double format for '%s' at line %d", key,
                 parser->lineno);
        return false;
    }

    *out = val;
    return true;
}

static bool parse_bool(const char *value, bool *out, DetScatParser *parser,
                       const char *key) {
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);
    assert(parser != NULL);
    assert(key != NULL && key[0] != '\0');

    if (str_raw_strcasecmp(value, "true") == 0) {
        *out = true;
    } else if (str_raw_strcasecmp(value, "false") == 0) {
        *out = false;
    } else {
        parser->status = DETSCAT_PARSER_ERR_FORMAT;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Invalid bool format for '%s' at line %d", key,
                 parser->lineno);
        return false;
    }

    return true;
}

static bool detscat_cfg_parse_line(DetScatParser *parser, DetScatCfg *cfg) {
    assert(parser != NULL);
    assert(cfg != NULL);

    char *trimmed = str_raw_trim(parser->line.data);
    if (trimmed[0] == '\0' || trimmed[0] == '#') return true;

    char *equals = strchr(trimmed, '=');
    if (!equals) {
        parser->status = DETSCAT_PARSER_ERR_FORMAT;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Missing '=' at line %d", parser->lineno);
        return false;
    }

    *equals = '\0';
    char *key = parser->line.data;
    char *value = equals + 1;

    key = str_raw_trim(key);
    value = str_raw_trim(value);

    if (strcmp(key, "is_polarized") == 0) {
        if (!parse_bool(value, &cfg->is_polarized, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "polarization") == 0) {
        int matched =
            sscanf(value, "[ [ %lf , %lf ] , [ %lf , %lf ] , [ %lf , %lf ] ]",
                   &cfg->polarization.x.re, &cfg->polarization.x.im,
                   &cfg->polarization.y.re, &cfg->polarization.y.im,
                   &cfg->polarization.z.re, &cfg->polarization.z.im);
        if (matched != 6) {
            parser->status = DETSCAT_PARSER_ERR_FORMAT;
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Invalid array format for '%s' at line %d", key,
                     parser->lineno);
            return false;
        }
    } else if (strcmp(key, "wavelength_nm") == 0) {
        if (!parse_double(value, &cfg->wavelength_nm, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "pulse_energy_mj") == 0) {
        if (!parse_double(value, &cfg->pulse_energy_mj, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "pulse_width_ns") == 0) {
        if (!parse_double(value, &cfg->pulse_width_ns, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "beam_diameter_mm") == 0) {
        if (!parse_double(value, &cfg->beam_diameter_mm, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "particles_definition_file") == 0) {
        if (strlen(value) > DETSCAT_CFG_PATH_MAX - 1) {
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Path too long for '%s' at line %d (max %zu chars)", key,
                     parser->lineno, DETSCAT_CFG_PATH_MAX - 1);
            return false;
        }
        if (!str_set(&cfg->particles_file, value)) {
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Could not set '%s' at line %d", key, parser->lineno);
            return false;
        }
    } else if (strcmp(key, "camera_center_position_m") == 0) {
        int matched =
            sscanf(value, "[ %lf , %lf , %lf ]", &cfg->camera_center_pos_m.x,
                   &cfg->camera_center_pos_m.y, &cfg->camera_center_pos_m.z);
        if (matched != 3) {
            parser->status = DETSCAT_PARSER_ERR_FORMAT;
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Invalid array format for '%s' at line %d", key,
                     parser->lineno);
            return false;
        }
    } else if (strcmp(key, "camera_sensor_normal_vector") == 0) {
        int matched =
            sscanf(value, "[ %lf , %lf , %lf ]", &cfg->camera_sensor_normal.x,
                   &cfg->camera_sensor_normal.y, &cfg->camera_sensor_normal.z);
        if (matched != 3) {
            parser->status = DETSCAT_PARSER_ERR_FORMAT;
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Invalid array format for '%s' at line %d", key,
                     parser->lineno);
            return false;
        }
    } else if (strcmp(key, "sensor_width_mm") == 0) {
        if (!parse_double(value, &cfg->sensor_width_mm, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "sensor_height_mm") == 0) {
        if (!parse_double(value, &cfg->sensor_height_mm, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "focal_length_mm") == 0) {
        if (!parse_double(value, &cfg->focal_length_mm, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "camera_resolution_x_px") == 0) {
        if (!parse_int(value, &cfg->camera_res_x_px, parser, key)) {
            return false;
        }
    } else if (strcmp(key, "camera_resolution_y_px") == 0) {
        if (!parse_int(value, &cfg->camera_res_y_px, parser, key)) {
            return false;
        }
    } else {
        parser->status = DETSCAT_PARSER_ERR_UNKNOWN_KEY;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Unknown key '%s' at line %d", key, parser->lineno);
        return false;
    }

    parser->status = DETSCAT_PARSER_OK;
    return true;
}

bool detscat_cfg_init(DetScatCfg *cfg, DetScatDiagnose *diag) {
    assert(cfg != NULL);

    memset(cfg, 0, sizeof(*cfg));

    if (!str_init(&cfg->particles_file)) {
        DETSCAT_SET_DIAGNOSE(*diag, DETSCAT_ERR_ALLOC, "%s",
                             "Failed to initialize particles file path");
        return false;
    }

    if (!str_reserve(&cfg->particles_file, DETSCAT_CFG_PATH_INIT)) {
        str_free(&cfg->particles_file);
        DETSCAT_SET_DIAGNOSE(*diag, DETSCAT_ERR_ALLOC, "%s",
                             "Failed to reserve memory for particles file path");
        return false;
    }

    return true;
}

void detscat_cfg_free(DetScatCfg *cfg) {
    if (!cfg) return;
    str_free(&cfg->particles_file);
    memset(cfg, 0, sizeof(*cfg));
}

bool detscat_cfg_load(const char *file_path, DetScatCfg *cfg,
                      DetScatDiagnose *diag) {
    assert(cfg != NULL);
    assert(diag != NULL);

    if (!file_path || file_path[0] == '\0') {
        DETSCAT_SET_DIAGNOSE(*diag, DETSCAT_ERR_INVALID_ARG, "%s",
                             "Function argument 'file_path' is invalid");
        return false;
    }

    DetScatParser parser;
    if (!detscat_parser_init(&parser, file_path)) {
        DETSCAT_SET_DIAGNOSE(*diag, DETSCAT_ERR_PARSING,
                             "Could not initialize parser: %s", parser.errmsg);
        return false;
    }

    while (detscat_parser_next_line(&parser)) {
        if (!detscat_cfg_parse_line(&parser, cfg)) {
            detscat_parser_free(&parser);
            DETSCAT_SET_DIAGNOSE(*diag, DETSCAT_ERR_PARSING,
                                 "Could not parse '%s': %s", file_path,
                                 parser.errmsg);
            return false;
        }
    }
    detscat_parser_free(&parser);

    detscat_log_info("Successfully parsed '%s'", file_path);
    return true;
}
