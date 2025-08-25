#ifndef DETSCAT_CONFIG_H
#define DETSCAT_CONFIG_H

#include <stdbool.h>
#include <stdio.h>

#include "detscat_diag.h"
#include "detscat_parser.h"

#include "mymath.h"
#include "str.h"

typedef struct {
    ComplexVec3     polarization;
    Vec3            camera_center_pos_m;
    Vec3            camera_sensor_normal;
    Str             particles_file;
    double          beam_diameter_mm;
    double          focal_length_mm;
    double          sensor_height_mm;
    double          sensor_width_mm;
    double          pulse_energy_mj;
    double          pulse_width_ns;
    double          wavelength_nm;
    int             camera_res_x_px;
    int             camera_res_y_px;
    bool            is_polarized;
} DetScatCfg;

extern DetScatCfg cfg;

bool detscat_cfg_load(const char *file_path, DetScatCfg *cfg, DetScatDiagnose *diag);
bool detscat_cfg_parse_line(DetScatParser *parser, DetScatCfg *cfg, DetScatDiagnose *diag);

#endif  // DETSCAT_CONFIG_H
