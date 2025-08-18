#ifndef DETSCAT_CAMERA_H
#define DETSCAT_CAMERA_H

#include "detscat_config.h"
#include "mymath.h"

typedef struct {
    Vec3 C;  // Camera center position in world coordinates
    Vec3 r;  // Right (unit) vector (local x-dimension) in world coordinates
    Vec3 u;  // Up (unit) vector (local y-dimension) in world coordinates
    Vec3 n;  // Optical axis (unit) vector (local z-dimension) in world
             // coordinates. Pointing from camera towards scene.

    double f;    // Camera focal length (meters)
    double p_x;  // Camera sensor pixel size in x (meters)
    double p_y;  // Camera sensor pixel size in y (meters)
    double
        c_x;  // Camera sensor principal point x (usually (width - 1) / 2) in px
    double c_y;  // Camera sensor principal point y (usually (height - 1) / 2)
                 // in px
                 //
    int width;   // Camera sensor number of pixels horizontally
    int height;  // Camera sensor number of pixels vertically
} DetScatCamera;

typedef struct {
    int width;
    int height;
    float *pixels;  // Grayscale floats
} DetScatImage;

void detscat_camera_init(DetScatCamera *camera, DetScatConfig *cfg);

int detscat_camera_image_create(DetScatImage *image, int w, int h);

int detscat_camera_get_image_index(DetScatImage *img, int u, int v);

void detscat_camera_pixel_coordinate_to_world(const DetScatCamera *camera,
                                              Vec3 *p, int u, int v);

void detscat_camera_pixel_observation_direction(const DetScatCamera *camera,
                                                Vec3 *d, int u, int v);

#endif  // DETSCAT_CAMERA_H
