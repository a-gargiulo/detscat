#ifndef DETSCAT_CAMERA_H
#define DETSCAT_CAMERA_H

#include "detscat.h"

#include "detscat_cfg.h"
#include "detscat_math.h"
#include "detscat_transform.h"


#include <stdbool.h>

typedef struct {
    float *pixels;
} DetScatImage;


typedef struct {
    DetScatImage image;
    int width;
    int height;
    Vec3 C;
    Vec3 r;
    Vec3 u;
    Vec3 n;
    double f;
    double p_x;
    double p_y;
    double c_x;
    double c_y;
} DetScatCamera;

bool detscat_camera_init(DetScatCamera *cam, const DetScatConfig *cfg, const DetScatTransform *glob_t, DetScatError *err);

// int detscat_camera_image_create(DetScatImage *image, int w, int h);

// int detscat_camera_get_image_index(DetScatImage *img, int u, int v);

// void detscat_camera_pixel_coordinate_to_world(const DetScatCamera *camera,
//                                               Vec3 *p, int u, int v);

// void detscat_camera_pixel_observation_direction(const DetScatCamera *camera,
//                                                 Vec3 *d, int u, int v);

#endif  // DETSCAT_CAMERA_H
