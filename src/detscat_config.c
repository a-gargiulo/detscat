#include "detscat_config.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strutil.h"

DetScatConfigParser *detscat_config_parser_create(const char *cfg_file_path) {
    DetScatConfigParser *cfg_parser = malloc(sizeof(DetScatConfigParser));
    if (!cfg_parser) return NULL;

    cfg_parser->file = fopen(cfg_file_path, "r");
    if (!cfg_parser->file) {
        free(cfg_parser);
        return NULL;
    }

    cfg_parser->line_number = 0;
    cfg_parser->eof = false;
    cfg_parser->status = DETSCAT_CONFIG_PARSER_OK;
    cfg_parser->line[0] = '\0';
    cfg_parser->err_msg[0] = '\0';

    return cfg_parser;
}

static bool parse_int(const char *value, int *out,
                      DetScatConfigParser *cfg_parser, const char *key) {
    assert(cfg_parser != NULL);
    assert(key != NULL && key[0] != '\0');
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);

    errno = 0;
    char *endptr = NULL;

    long val = strtol(value, &endptr, 10);
    if (errno != 0) {
        cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_VAL_RANGE;
        snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                 "Overflow/underflow parsing '%s' on line %d", key,
                 cfg_parser->line_number);
        return false;
    }

    if (endptr == value || *endptr != '\0') {
        cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
        snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                 "Invalid integer format for '%s' on line %d", key,
                 cfg_parser->line_number);
        return false;
    }

    // safety check before casting long to int
    if (val < INT_MIN || val > INT_MAX) {
        cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_VAL_RANGE;
        snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                 "Integer value out of range for '%s' on line %d", key,
                 cfg_parser->line_number);
        return false;
    }

    *out = (int)val;
    return true;
}

static bool parse_double(const char *value, double *out,
                         DetScatConfigParser *cfg_parser, const char *key) {
    assert(cfg_parser != NULL);
    assert(key != NULL && key[0] != '\0');
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);

    errno = 0;
    char *endptr = NULL;
    double val = strtod(value, &endptr);

    if (errno != 0) {
        cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_VAL_RANGE;
        snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                 "Overflow/underflow parsing '%s' on line %d", key,
                 cfg_parser->line_number);
        return false;
    }

    if (endptr == value || *endptr != '\0') {
        cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
        snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                 "Invalid double format for '%s' on line %d", key,
                 cfg_parser->line_number);
        return false;
    }

    *out = val;
    return true;
}

static bool parse_bool(const char *value, bool *out,
                       DetScatConfigParser *cfg_parser, const char *key) {
    assert(cfg_parser != NULL);
    assert(key != NULL && key[0] != '\0');
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);

    if (strutil_strcasecmp(value, "true") == 0) {
        *out = true;
    } else if (strutil_strcasecmp(value, "false") == 0) {
        *out = false;
    } else {
        cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
        snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                 "Invalid bool format for '%s' on line %d", key,
                 cfg_parser->line_number);
        return false;
    }

    return true;
}

bool detscat_config_cfg_parser_parse(DetScatConfigParser *cfg_parser,
                                     DetScatConfig *config) {
    assert(cfg_parser != NULL);
    assert(config != NULL);

    while (
        fgets(cfg_parser->line, sizeof(cfg_parser->line), cfg_parser->file)) {
        cfg_parser->line_number++;

        char *trimmed = strutil_trim(cfg_parser->line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

        char *equals = strchr(trimmed, '=');
        if (!equals) {
            cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
            snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                     "Missing '=' on line %d", cfg_parser->line_number);
            return false;
        }

        *equals = '\0';
        char *key = cfg_parser->line;
        char *value = equals + 1;

        key = strutil_trim(key);
        value = strutil_trim(value);

        if (strcmp(key, "is_polarized") == 0) {
            if (!parse_bool(value, &config->is_polarized, cfg_parser, key)) {
                return false;
            }
        } else if (strcmp(key, "polarization") == 0) {
            int matched = sscanf(
                value, "[ [ %lf , %lf ] , [ %lf , %lf ] , [ %lf , %lf ] ]",
                &config->polarization.x.re, &config->polarization.x.im,
                &config->polarization.y.re, &config->polarization.y.im,
                &config->polarization.z.re, &config->polarization.z.im);
            if (matched != 6) {
                cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
                snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                         "Failed to parse 'polarization' on line %d",
                         cfg_parser->line_number);
                return false;
            }
        } else if (strcmp(key, "wavelength_nm") == 0) {
            if (!parse_double(value, &config->wavelength_nm, cfg_parser, key)) {
                return false;
            }
        } else if (strcmp(key, "pulse_energy_mj") == 0) {
            if (!parse_double(value, &config->pulse_energy_mj, cfg_parser,
                              key)) {
                return false;
            }
        } else if (strcmp(key, "pulse_width_ns") == 0) {
            if (!parse_double(value, &config->pulse_width_ns, cfg_parser,
                              key)) {
                return false;
            }
        } else if (strcmp(key, "beam_diameter_mm") == 0) {
            if (!parse_double(value, &config->beam_diameter_mm, cfg_parser,
                              key)) {
                return false;
            }
        } else if (strcmp(key, "particles_definition_file") == 0) {
            strncpy(config->particles_definition_file, value,
                    DETSCAT_CONFIG_PATH_MAX - 1);
            config->particles_definition_file[DETSCAT_CONFIG_PATH_MAX - 1] =
                '\0';
        } else if (strcmp(key, "camera_center_position_m") == 0) {
            int matched = sscanf(value, "[ %lf , %lf , %lf ]",
                                 &config->camera_center_position_m.x,
                                 &config->camera_center_position_m.y,
                                 &config->camera_center_position_m.z);
            if (matched != 3) {
                cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
                snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                         "Failed to parse 'camera_center_position' on line %d",
                         cfg_parser->line_number);
                return false;
            }
        } else if (strcmp(key, "camera_sensor_normal_vector") == 0) {
            int matched = sscanf(value, "[ %lf , %lf , %lf ]",
                                 &config->camera_sensor_normal_vector.x,
                                 &config->camera_sensor_normal_vector.y,
                                 &config->camera_sensor_normal_vector.z);
            if (matched != 3) {
                cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
                snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                         "Failed to parse 'camera_sensor_normal' on line %d",
                         cfg_parser->line_number);
                return false;
            }
        } else if (strcmp(key, "sensor_width_mm") == 0) {
            if (!parse_double(value, &config->sensor_width_mm, cfg_parser,
                              key)) {
                return false;
            }
        } else if (strcmp(key, "sensor_height_mm") == 0) {
            if (!parse_double(value, &config->sensor_height_mm, cfg_parser,
                              key)) {
                return false;
            }
        } else if (strcmp(key, "focal_length_mm") == 0) {
            if (!parse_double(value, &config->focal_length_mm, cfg_parser,
                              key)) {
                return false;
            }
        } else if (strcmp(key, "camera_resolution_x_px") == 0) {
            if (!parse_int(value, &config->camera_resolution_x_px, cfg_parser,
                           key)) {
                return false;
            }
        } else if (strcmp(key, "camera_resolution_y_px") == 0) {
            if (!parse_int(value, &config->camera_resolution_y_px, cfg_parser,
                           key)) {
                return false;
            }
        } else {
            cfg_parser->status = DETSCAT_CONFIG_PARSER_ERR_UNKNOWN_KEY;
            snprintf(cfg_parser->err_msg, sizeof(cfg_parser->err_msg),
                     "Unknown key '%s' on line %d", key,
                     cfg_parser->line_number);
            return false;
        }
    }
    cfg_parser->eof = true;
    cfg_parser->status = DETSCAT_CONFIG_PARSER_OK;
    return true;
}

void detscat_config_cfg_parser_free(DetScatConfigParser *cfg_parser) {
    if (!cfg_parser) return;

    if (cfg_parser->file) {
        fclose(cfg_parser->file);
        cfg_parser->file = NULL;
    }

    free(cfg_parser);
}
