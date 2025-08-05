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
    assert(cfg_file_path != NULL && cfg_file_path[0] != '\0');

    DetScatConfigParser *parser = malloc(sizeof(DetScatConfigParser));
    if (!parser) return NULL;

    parser->file = fopen(cfg_file_path, "r");
    if (!parser->file) {
        free(parser);
        return NULL;
    }

    parser->line_number = 0;
    parser->eof = false;
    parser->status = DETSCAT_CONFIG_PARSER_OK;
    parser->line[0] = '\0';
    parser->err_msg[0] = '\0';

    return parser;
}

static bool parse_int(const char *value, int *out, DetScatConfigParser *parser,
                      const char *key) {
    assert(parser != NULL);
    assert(key != NULL && key[0] != '\0');
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);

    errno = 0;
    char *endptr = NULL;

    long val = strtol(value, &endptr, 10);
    if (errno != 0) {
        parser->status = DETSCAT_CONFIG_PARSER_ERR_VAL_RANGE;
        snprintf(parser->err_msg, sizeof(parser->err_msg),
                 "Overflow/underflow parsing '%s' on line %d", key,
                 parser->line_number);
        return false;
    }

    if (endptr == value || *endptr != '\0') {
        parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
        snprintf(parser->err_msg, sizeof(parser->err_msg),
                 "Invalid integer format for '%s' on line %d", key,
                 parser->line_number);
        return false;
    }

    // safety check before casting long to int
    if (val < INT_MIN || val > INT_MAX) {
        parser->status = DETSCAT_CONFIG_PARSER_ERR_VAL_RANGE;
        snprintf(parser->err_msg, sizeof(parser->err_msg),
                 "Integer value out of range for '%s' on line %d", key,
                 parser->line_number);
        return false;
    }

    *out = (int)val;
    return true;
}

static bool parse_double(const char *value, double *out,
                         DetScatConfigParser *parser, const char *key) {
    assert(parser != NULL);
    assert(key != NULL && key[0] != '\0');
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);

    errno = 0;
    char *endptr = NULL;
    double val = strtod(value, &endptr);

    if (errno != 0) {
        parser->status = DETSCAT_CONFIG_PARSER_ERR_VAL_RANGE;
        snprintf(parser->err_msg, sizeof(parser->err_msg),
                 "Overflow/underflow parsing '%s' on line %d", key,
                 parser->line_number);
        return false;
    }

    if (endptr == value || *endptr != '\0') {
        parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
        snprintf(parser->err_msg, sizeof(parser->err_msg),
                 "Invalid double format for '%s' on line %d", key,
                 parser->line_number);
        return false;
    }

    *out = val;
    return true;
}

static bool parse_bool(const char *value, bool *out,
                       DetScatConfigParser *parser, const char *key) {
    assert(parser != NULL);
    assert(key != NULL && key[0] != '\0');
    assert(value != NULL && value[0] != '\0');
    assert(out != NULL);

    if (strutil_strcasecmp(value, "true") == 0) {
        *out = true;
    } else if (strutil_strcasecmp(value, "false") == 0) {
        *out = false;
    } else {
        parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
        snprintf(parser->err_msg, sizeof(parser->err_msg),
                 "Invalid bool format for '%s' on line %d", key,
                 parser->line_number);
        return false;
    }

    return true;
}

bool detscat_config_parser_parse(DetScatConfigParser *parser,
                                 DetScatConfig *config) {
    assert(parser != NULL);
    assert(config != NULL);

    while (fgets(parser->line, sizeof(parser->line), parser->file)) {
        parser->line_number++;

        char *trimmed = strutil_trim(parser->line);
        if (trimmed[0] == '\0' || trimmed[0] == '#') continue;

        char *equals = strchr(trimmed, '=');
        if (!equals) {
            parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
            snprintf(parser->err_msg, sizeof(parser->err_msg),
                     "Missing '=' on line %d", parser->line_number);
            return false;
        }

        *equals = '\0';
        char *key = parser->line;
        char *value = equals + 1;

        key = strutil_trim(key);
        value = strutil_trim(value);

        if (strcmp(key, "is_polarized") == 0) {
            if (!parse_bool(value, &config->is_polarized, parser, key)) {
                return false;
            }
        } else if (strcmp(key, "polarization") == 0) {
            int matched = sscanf(
                value, "[ [ %lf , %lf ] , [ %lf , %lf ] , [ %lf , %lf ] ]",
                &config->polarization.x.re, &config->polarization.x.im,
                &config->polarization.y.re, &config->polarization.y.im,
                &config->polarization.z.re, &config->polarization.z.im);
            if (matched != 6) {
                parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
                snprintf(parser->err_msg, sizeof(parser->err_msg),
                         "Failed to parse 'polarization' on line %d",
                         parser->line_number);
                return false;
            }
        } else if (strcmp(key, "wavelength_nm") == 0) {
            if (!parse_double(value, &config->wavelength_nm, parser, key)) {
                return false;
            }
        } else if (strcmp(key, "pulse_energy_mj") == 0) {
            if (!parse_double(value, &config->pulse_energy_mj, parser, key)) {
                return false;
            }
        } else if (strcmp(key, "pulse_width_ns") == 0) {
            if (!parse_double(value, &config->pulse_width_ns, parser, key)) {
                return false;
            }
        } else if (strcmp(key, "beam_diameter_mm") == 0) {
            if (!parse_double(value, &config->beam_diameter_mm, parser, key)) {
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
                parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
                snprintf(parser->err_msg, sizeof(parser->err_msg),
                         "Failed to parse 'camera_center_position' on line %d",
                         parser->line_number);
                return false;
            }
        } else if (strcmp(key, "camera_sensor_normal_vector") == 0) {
            int matched = sscanf(value, "[ %lf , %lf , %lf ]",
                                 &config->camera_sensor_normal_vector.x,
                                 &config->camera_sensor_normal_vector.y,
                                 &config->camera_sensor_normal_vector.z);
            if (matched != 3) {
                parser->status = DETSCAT_CONFIG_PARSER_ERR_FORMAT;
                snprintf(parser->err_msg, sizeof(parser->err_msg),
                         "Failed to parse 'camera_sensor_normal' on line %d",
                         parser->line_number);
                return false;
            }
        } else if (strcmp(key, "sensor_width_mm") == 0) {
            if (!parse_double(value, &config->sensor_width_mm, parser, key)) {
                return false;
            }
        } else if (strcmp(key, "sensor_height_mm") == 0) {
            if (!parse_double(value, &config->sensor_height_mm, parser, key)) {
                return false;
            }
        } else if (strcmp(key, "focal_length_mm") == 0) {
            if (!parse_double(value, &config->focal_length_mm, parser, key)) {
                return false;
            }
        } else if (strcmp(key, "camera_resolution_x_px") == 0) {
            if (!parse_int(value, &config->camera_resolution_x_px, parser,
                           key)) {
                return false;
            }
        } else if (strcmp(key, "camera_resolution_y_px") == 0) {
            if (!parse_int(value, &config->camera_resolution_y_px, parser,
                           key)) {
                return false;
            }
        } else {
            parser->status = DETSCAT_CONFIG_PARSER_ERR_UNKNOWN_KEY;
            snprintf(parser->err_msg, sizeof(parser->err_msg),
                     "Unknown key '%s' on line %d", key, parser->line_number);
            return false;
        }
    }
    parser->eof = true;
    parser->status = DETSCAT_CONFIG_PARSER_OK;
    return true;
}

void detscat_config_parser_free(DetScatConfigParser *parser) {
    if (!parser) return;

    if (parser->file) {
        fclose(parser->file);
        parser->file = NULL;
    }

    free(parser);
}
