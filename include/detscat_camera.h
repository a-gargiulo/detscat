#ifndef DETSCAT_CAMERA_H
#define DETSCAT_CAMERA_H

#include "detscat_config.h"
#include "mymath.h"

typedef struct {
    Vec3 C;             // Camera center position in world coordinates Vec3 r;             // Right vector (unit vector)
    Vec3 r;
    Vec3 u;             // up vector (unit vector)
    Vec3 n;             // Optical axis (unit vector): from camera toward scene: world units
    double f;           // Focal length (meters)
    double p_x;         // Pixel size in x (meters)
    double p_y;         // Pixel size in y (meters)
    int width;          // Number of pixels horizontally
    int height;         // Number of pixels vertically
    double c_x;         // Principal point x (usually (width - 1) / 2) in px
    double c_y;         // Principal point y (usually (height - 1) / 2) in px
} DetScatCamera;

typedef struct {
    int width;
    int height;
    float *pixels; // Grayscale floats
} DetScatImage;


void detscat_camera_camera_create(DetScatCamera *camera, DetScatConfig *cfg);

int detscat_camera_image_create(DetScatImage *image, int w, int h);

int detscat_camera_get_image_index(DetScatImage* img, int u, int v);

void detscat_camera_pixel_coordinate_to_world(const DetScatCamera *camera, Vec3 *p, int u, int v);

void detscat_camera_pixel_observation_direction(const DetScatCamera* camera, Vec3 *d, int u, int v);

#endif  // DETSCAT_CAMERA_H
