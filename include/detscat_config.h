#ifndef DETSCAT_CONFIG_H
#define DETSCAT_CONFIG_H

#include <stdbool.h>

#include "mymath.h"

#define DETSCAT_CONFIG_PATH_MAX 1024

typedef struct
{
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

#endif // DETSCAT_CONFIG_H
