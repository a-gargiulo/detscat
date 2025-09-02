#ifndef DETSCAT_CFG_H
#define DETSCAT_CFG_H

#include "detscat_diag.h"
#include "detscat_math.h"
#include "detscat_str.h"

#include <stdbool.h>

//------------------------------------------------------------------------------
// Data structures 
//------------------------------------------------------------------------------

typedef struct DetScatConfig {

    // I/O parameters 
    Str            particles_file_path;

    // Laser parameters
    ComplexVec3    polarization;
    double         wavelength_nm;
    double         pulse_energy_mj;
    double         pulse_width_ns;
    double         beam_diameter_mm;
    bool           is_polarized;

    // Camera parameters
    Vec3           camera_center_position_m;
    Vec3           camera_sensor_normal;
    double         focal_length_mm;
    double         sensor_width_mm;
    double         sensor_height_mm;
    int            camera_resolution_x_px;
    int            camera_resolution_y_px;

} DetScatConfig;

//------------------------------------------------------------------------------
// Public API 
//------------------------------------------------------------------------------

bool detscat_cfg_create(DetScatConfig **cfg, DetScatDiagnose *diag);
void detscat_cfg_destroy(DetScatConfig **cfg);
bool detscat_cfg_load(const char *file_path, DetScatConfig *cfg,
                      DetScatDiagnose *diag);

#endif  // DETSCAT_CFG_H
