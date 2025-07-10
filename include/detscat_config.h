#ifndef DETSCAT_CONFIG_H
#define DETSCAT_CONFIG_H

#include <stdbool.h>
#include <stdio.h>

#include "mymath.h"

#define DETSCAT_CONFIG_LINE_MAX 1024
#define DETSCAT_CONFIG_PATH_MAX 512
#define DETSCAT_CONFIG_PARSER_ERR_MSG_MAX 256

typedef struct {
    // INCIDENT LIGHT SOURCE (LASER)
    bool is_polarized;
    ComplexVec3 polarization;
    double wavelength_nm;
    double pulse_energy_mj;
    double pulse_width_ns;
    double beam_diameter_mm;

    // PARTICLES
    char particles_definition_file[DETSCAT_CONFIG_PATH_MAX];

    // CAMERA
    Vec3 camera_center_position_m;
    Vec3 camera_sensor_normal_vector;
    double sensor_width_mm;
    double sensor_height_mm;
    double focal_length_mm;
    int camera_resolution_x_px;
    int camera_resolution_y_px;

} DetScatConfig;

extern DetScatConfig config;

typedef enum {
    DETSCAT_CONFIG_PARSER_OK = 0,
    DETSCAT_CONFIG_PARSER_ERR_FILE_NOT_FOUND,
    DETSCAT_CONFIG_PARSER_ERR_ALLOC,
    DETSCAT_CONFIG_PARSER_ERR_INVALID_ARG,
    DETSCAT_CONFIG_PARSER_ERR_FORMAT,
    DETSCAT_CONFIG_PARSER_ERR_UNKNOWN_KEY,
} DetScatConfigParserStatus;

typedef struct {
    FILE *file;                                             // Open config file
    char line[DETSCAT_CONFIG_LINE_MAX];                     // Current line buffer
    int line_number;                                        // Current line number
    bool eof;                                               // End-of-file flag
    DetScatConfigParserStatus status;                       // Parser status code
    char error_message[DETSCAT_CONFIG_PARSER_ERR_MSG_MAX];  // Last error message
} DetScatConfigParser;

// Open and initialize parser from file path; returns parser or NULL on failure.
DetScatConfigParser *detscat_config_parser_create(const char *file_path);

// Parse file into config struct; returns true on success, false on error.
bool detscat_config_parser_parse(DetScatConfigParser *parser, DetScatConfig *config);

// Close file and free parser memory.
void detscat_config_parser_free(DetScatConfigParser *parser);

#endif  // DETSCAT_CONFIG_H
