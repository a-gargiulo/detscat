#ifndef DETSCAT_CAMERA_H
#define DETSCAT_CAMERA_H

#include "detscat.h"

#include "detscat_cfg.h"
#include "detscat_math.h"
#include "detscat_transform.h"


#include <stdbool.h>

typedef struct {
    float *pixels;
    int width;
    int height;
} DetScatImage;

typedef struct {
    double f;
    double p_x;
    double p_y;
    double c_x;
    double c_y;
} DetScatCameraIntrinsic;

typedef struct {
    Vec3 translation;
    Mat3 rotation;
} DetScatCameraExtrinsic;

typedef struct {
    Vec3 r;
    Vec3 u;
    Vec3 n;
} DetScatCameraAxes;


typedef struct {
    DetScatImage image;
    DetScatCameraAxes axes;
    DetScatCameraIntrinsic intrinsics;
    DetScatCameraExtrinsic extrinsics;
} DetScatCamera;

bool detscat_camera_init(DetScatCamera *cam, const DetScatConfig *cfg, const DetScatTransform *glob_t, DetScatError *err);

void detscat_camera_free(DetScatCamera *cam);

int detscat_camera_get_pixel_index(int u, int v, const DetScatImage *img);

void detscat_camera_pixel_coordinate_to_world(Vec3 *p, int u, int v, const DetScatCamera *cam);

// void detscat_camera_pixel_observation_direction(const DetScatCamera *camera,
//                                                 Vec3 *d, int u, int v);

#endif  // DETSCAT_CAMERA_H
