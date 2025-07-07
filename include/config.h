#ifndef DETSCAT_CONFIG_H
#define DETSCAT_CONFIG_H

#include "mymath.h"

#define MAX_PATH_LENGTH 256
#define MAX_TYPE_LENGTH 32

typedef struct {
    char type[MAX_TYPE_LENGTH];
    ComplexVec3 polarization;
    double wavelength_nm;
    double pulse_energy_mj;
    double pulse_width_ns;
    double beam_diameter_mm;
    char particle_file_path[MAX_PATH_LENGTH];
    Vec3 camera_center_position;
    Vec3 camera_sensor_normal;
    double sensor_width_mm;
    double sensor_height_mm;
    double focal_length_mm;
    int camera_resolution_x;
    int camera_resolution_y;
} Config;

extern Config config;

#endif  //  DETSCAT_CONFIG_H
