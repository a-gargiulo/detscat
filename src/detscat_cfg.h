#ifndef DETSCAT_CFG_H
#define DETSCAT_CFG_H

#include "detscat.h"

#include "detscat_math.h"
#include "detscat_str.h"

#include <stdbool.h>

typedef struct {

    // Light source
    Vec3           light_source_position_m;
    Vec3           light_source_direction;
    double         beam_diameter_mm;
    double         wavelength_nm;
    Str            polarization_type;
    Str            polarization_axis;
    double         elliptical_alpha_deg;
    double         elliptical_beta_deg;
    double         pulse_energy_mj;
    double         pulse_width_ns;

    // Particles
    Str            particles_file_path;

    // Camera
    Vec3           camera_center_position_m;
    Vec3           camera_direction;
    double         focal_length_mm;
    double         sensor_width_mm;
    double         sensor_height_mm;
    int            sensor_resolution_x_px;
    int            sensor_resolution_y_px;

} DetScatConfig;

bool detscat_cfg_init(DetScatConfig *cfg, DetScatError *err);
void detscat_cfg_free(DetScatConfig *cfg);
bool detscat_cfg_load(const char *cfg_file_path, DetScatConfig *cfg,
                      DetScatError *err);

#endif  // DETSCAT_CFG_H
