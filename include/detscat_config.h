#ifndef DETSCAT_CONFIG_H
#define DETSCAT_CONFIG_H

#include <stdbool.h>
#include <stdio.h>

#include "mymath.h"

// The cfg file line will be truncated if it's too large,
// which will lead to failure later in the program execution.
#define DETSCAT_CFG_LINE_MAX 1024
// Bound check implemented for cfg paths.
// The program will fail immediately if the path is too long
#define DETSCAT_CFG_PATH_MAX 512
// Error messages will be truncated if they are too large, as
// the error message buffers are strictly filled using 'snprintf'
#define DETSCAT_CFG_ERRMSG_MAX 256

typedef struct {
    ComplexVec3 polarization;
    Vec3 camera_center_pos_m;
    Vec3 camera_sensor_normal;
    double beam_diameter_mm;
    double focal_length_mm;
    double sensor_height_mm;
    double sensor_width_mm;
    double pulse_energy_mj;
    double pulse_width_ns;
    double wavelength_nm;
    int camera_res_x_px;
    int camera_res_y_px;
    bool is_polarized;
    char particles_file[DETSCAT_CFG_PATH_MAX];
} DetScatCfg;

extern DetScatCfg cfg;

typedef enum {
    DETSCAT_CFG_PARSER_OK = 0,
    DETSCAT_CFG_PARSER_ERR_FORMAT,
    DETSCAT_CFG_PARSER_ERR_UNKNOWN_KEY,
    DETSCAT_CFG_PARSER_ERR_RANGE,
} DetScatCfgParserStatus;

typedef struct {
    FILE *file;
    int line_number;
    DetScatCfgParserStatus status;
    bool eof;
    char line[DETSCAT_CFG_LINE_MAX];
    char errmsg[DETSCAT_CFG_ERRMSG_MAX];
} DetScatCfgParser;

bool detscat_cfg_parser_init(DetScatCfgParser *parser, const char *file_path);
bool detscat_cfg_parser_load(DetScatCfgParser *parser, DetScatCfg *cfg);
void detscat_cfg_parser_close(DetScatCfgParser *parser);

#endif  // DETSCAT_CONFIG_H
