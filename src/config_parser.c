#include "config_parser.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strutil.h"

ConfigParser *detscat_config_parser_init(const char *config_file_path)
{
    ConfigParser *parser = malloc(sizeof(ConfigParser));
    if (!parser)
        return NULL;

    parser->file = fopen(config_file_path, "r");
    if (!parser->file)
    {
        free(parser);
        return NULL;
    }

    parser->line_number = 0;
    parser->eof = false;
    parser->error_code = CONFIG_PARSER_OK;
    parser->line[0] = '\0';

    return parser;
}

bool parse_int(const char *value, int *out, ConfigParser *parser, const char *key)
{
    if (!value || !out || !parser || !key)
    {
        if (parser)
        {
            parser->error_code = CONFIG_PARSER_ERR_INVALID_ARGUMENT;
            snprintf(parser->error_message, sizeof(parser->error_message),
                     "Internal error: invalid argument to 'parse_int'");
        }
        return false;
    }

    errno = 0;
    char *endptr = NULL;
    long val = strtol(value, &endptr, 10);

    if (errno != 0)
    {
        parser->error_code = CONFIG_PARSER_ERR_INVALID_FORMAT;
        snprintf(parser->error_message, sizeof(parser->error_message), "Overflow/underflow parsing '%s' on line %d",
                 key, parser->line_number);
        return false;
    }

    if (endptr == value || *endptr != '\0')
    {
        parser->error_code = CONFIG_PARSER_ERR_INVALID_FORMAT;
        snprintf(parser->error_message, sizeof(parser->error_message), "Invalid integer format for '%s' on line %d",
                 key, parser->line_number);
        return false;
    }

    if (val < INT_MIN || val > INT_MAX)
    {
        parser->error_code = CONFIG_PARSER_ERR_INVALID_FORMAT;
        snprintf(parser->error_message, sizeof(parser->error_message), "Integer value out of range for '%s' on line %d",
                 key, parser->line_number);
        return false;
    }

    *out = (int)val;
    return true;
}

bool parse_double(const char *value, double *out, ConfigParser *parser, const char *key)
{
    if (!value || !out || !parser || !key)
    {
        if (parser)
        {
            parser->error_code = CONFIG_PARSER_ERR_INVALID_ARGUMENT;
            snprintf(parser->error_message, sizeof(parser->error_message),
                     "Internal error: invalid argument to 'parse_double'");
        }
        return false;
    }

    errno = 0;
    char *endptr = NULL;
    double val = strtod(value, &endptr);

    if (errno != 0)
    {
        parser->error_code = CONFIG_PARSER_ERR_INVALID_FORMAT;
        snprintf(parser->error_message, sizeof(parser->error_message), "Overflow/underflow parsing '%s' on line %d",
                 key, parser->line_number);
        return false;
    }

    if (endptr == value || *endptr != '\0')
    {
        parser->error_code = CONFIG_PARSER_ERR_INVALID_FORMAT;
        snprintf(parser->error_message, sizeof(parser->error_message), "Invalid double format for '%s' on line %d", key,
                 parser->line_number);
        return false;
    }

    *out = val;
    return true;
}

bool detscat_config_parser_parse(ConfigParser *parser, Config *config)
{
    if (!parser || !config)
    {
        if (parser)
            parser->error_code = CONFIG_PARSER_ERR_INVALID_ARGUMENT;
        snprintf(parser->error_message, sizeof(parser->error_message),
                 "Internal error: invalid argument to 'detscat_config_parser_parse'");
        return false;
    }

    memset(config, 0, sizeof(*config));

    while (fgets(parser->line, sizeof(parser->line), parser->file))
    {
        parser->line_number++;

        char *trimmed = strutil_trim(parser->line);
        if (trimmed[0] == '\0' || trimmed[0] == '#')
            continue;

        char *equals = strchr(trimmed, '=');
        if (!equals)
        {
            parser->error_code = CONFIG_PARSER_ERR_INVALID_FORMAT;
            snprintf(parser->error_message, sizeof(parser->error_message), "Missing '=' on line %d",
                     parser->line_number);
            return false;
        }

        *equals = '\0';
        char *key = parser->line;
        char *value = equals + 1;

        key = strutil_trim(key);
        value = strutil_trim(value);

        if (strcmp(key, "type") == 0)
        {
            strncpy(config->type, value, MAX_TYPE_LENGTH - 1);
            config->type[MAX_TYPE_LENGTH - 1] = '\0';
            if (strcmp(value, "polarized") != 0 && strcmp(value, "unpolarized") != 0)
            {
                parser->error_code = CONFIG_PARSER_ERR_INVALID_VALUE;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Invalid value '%s' for key '%s' on line %d. Expected 'polarized' or 'unpolarized'.", value,
                         key, parser->line_number);
                return false;
            }
        }
        else if (strcmp(key, "polarization") == 0)
        {
            int matched = sscanf(value, "[ [ %lf , %lf ] , [ %lf , %lf ] , [ %lf , %lf ] ]", &config->polarization.x.re,
                                 &config->polarization.x.im, &config->polarization.y.re, &config->polarization.y.im,
                                 &config->polarization.z.re, &config->polarization.z.im);
            if (matched != 6)
            {
                parser->error_code = CONFIG_PARSER_ERR_INVALID_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Failed to parse 'polarization' on line %d", parser->line_number);
                return false;
            }
        }
        else if (strcmp(key, "wavelength_nm") == 0)
        {
            if (!parse_double(value, &config->wavelength_nm, parser, key))
            {
                return false;
            }
        }
        else if (strcmp(key, "pulse_energy_mj") == 0)
        {
            if (!parse_double(value, &config->pulse_energy_mj, parser, key))
            {
                return false;
            }
        }
        else if (strcmp(key, "pulse_width_ns") == 0)
        {
            if (!parse_double(value, &config->pulse_width_ns, parser, key))
            {
                return false;
            }
        }
        else if (strcmp(key, "beam_diameter_mm") == 0)
        {
            if (!parse_double(value, &config->beam_diameter_mm, parser, key))
            {
                return false;
            }
        }
        else if (strcmp(key, "particle_file_path") == 0)
        {
            strncpy(config->particle_file_path, value, MAX_PATH_LENGTH - 1);
            config->particle_file_path[MAX_PATH_LENGTH - 1] = '\0';
        }
        else if (strcmp(key, "camera_center_position") == 0)
        {
            int matched = sscanf(value, "[ %lf , %lf , %lf ]", &config->camera_center_position.x,
                                 &config->camera_center_position.y, &config->camera_center_position.z);
            if (matched != 3)
            {
                parser->error_code = CONFIG_PARSER_ERR_INVALID_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Failed to parse 'camera_center_position' on line %d", parser->line_number);
                return false;
            }
        }
        else if (strcmp(key, "camera_sensor_normal") == 0)
        {
            int matched = sscanf(value, "[ %lf , %lf , %lf ]", &config->camera_sensor_normal.x,
                                 &config->camera_sensor_normal.y, &config->camera_sensor_normal.z);
            if (matched != 3)
            {
                parser->error_code = CONFIG_PARSER_ERR_INVALID_FORMAT;
                snprintf(parser->error_message, sizeof(parser->error_message),
                         "Failed to parse 'camera_sensor_normal' on line %d", parser->line_number);
                return false;
            }
        }
        else if (strcmp(key, "sensor_width_mm") == 0)
        {
            if (!parse_double(value, &config->sensor_width_mm, parser, key))
            {
                return false;
            }
        }
        else if (strcmp(key, "sensor_height_mm") == 0)
        {
            if (!parse_double(value, &config->sensor_height_mm, parser, key))
            {
                return false;
            }
        }
        else if (strcmp(key, "focal_length_mm") == 0)
        {
            if (!parse_double(value, &config->focal_length_mm, parser, key))
            {
                return false;
            }
        }
        else if (strcmp(key, "camera_resolution_x") == 0)
        {
            if (!parse_int(value, &config->camera_resolution_x, parser, key))
            {
                return false;
            }
        }
        else if (strcmp(key, "camera_resolution_y") == 0)
        {
            if (!parse_int(value, &config->camera_resolution_y, parser, key))
            {
                return false;
            }
        }
        else
        {
            parser->error_code = CONFIG_PARSER_ERR_UNKNOWN_KEY;
            snprintf(parser->error_message, sizeof(parser->error_message), "Unknown key '%s' on line %d", key,
                     parser->line_number);
            return false;
        }
    }
    parser->eof = true;
    parser->error_code = CONFIG_PARSER_OK;
    return true;
}

void detscat_config_parser_free(ConfigParser *parser)
{
    if (!parser)
        return;

    if (parser->file)
    {
        fclose(parser->file);
        parser->file = NULL;
    }

    free(parser);
}
