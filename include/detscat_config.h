#ifndef DETSCAT_CONFIG_H
#define DETSCAT_CONFIG_H

#include <stdbool.h>
#include <stdio.h>

#include "mymath.h"

#define DETSCAT_CONFIG_LINE_MAX 1024
#define DETSCAT_CONFIG_PATH_MAX 512
#define DETSCAT_CONFIG_ERR_MSG_MAX 256

typedef struct {

    ComplexVec3 polarization;

    Vec3 camera_center_position_m;
    Vec3 camera_sensor_normal_vector;

    double beam_diameter_mm;
    double focal_length_mm;
    double sensor_height_mm;
    double sensor_width_mm;
    double pulse_energy_mj;
    double pulse_width_ns;
    double wavelength_nm;

    int camera_resolution_x_px;
    int camera_resolution_y_px;

    bool is_polarized;

    char particles_definition_file[DETSCAT_CONFIG_PATH_MAX];

} DetScatConfig;

extern DetScatConfig config;

typedef enum {

    DETSCAT_CONFIG_PARSER_OK = 0,
    DETSCAT_CONFIG_PARSER_ERR_FORMAT,
    DETSCAT_CONFIG_PARSER_ERR_UNKNOWN_KEY,
    DETSCAT_CONFIG_PARSER_ERR_VAL_RANGE

} DetScatConfigParserStatus;

typedef struct {

    FILE *file;

    int line_number;

    DetScatConfigParserStatus status;

    bool eof;

    char line[DETSCAT_CONFIG_LINE_MAX];
    char err_msg[DETSCAT_CONFIG_ERR_MSG_MAX];

} DetScatConfigParser;

DetScatConfigParser *detscat_config_parser_create(const char *cfg_file_path);

bool detscat_config_parser_parse(DetScatConfigParser *cfg_parser,
                                 DetScatConfig *config);

void detscat_config_parser_free(DetScatConfigParser *cfg_parser);

#endif  // DETSCAT_CONFIG_H
