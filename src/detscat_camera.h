#ifndef DETSCAT_CAMERA_H
#define DETSCAT_CAMERA_H

#include "detscat_config.h"
#include "mymath.h"

typedef struct {
    Vec3 C;      /**< Camera center position (world) */
    Vec3 r;      /**< Camera 'right' x-direction (world)  */
    Vec3 u;      /**< Camera 'up' y-direction (world) */ 
    Vec3 n;      /**< Camera 'normal' z-direction (world) */
    double f;    /**< Camera focal length [m] */
    double p_x;  /**< Camera sensor pixel size in x [m] */
    double p_y;  /**< Camera sensor pixel size in y [m] */
    double c_x;  /**< Camera sensor principal point x [px] (image) */
    double c_y;  /**< Camera sensor principal point y [px] (image) */
    int width;   /**< Camera sensor number of pixels in x (image) */
    int height;  /**< Camera sensor number of pixels in y (image) */
} DetScatCamera;

typedef struct {
    int width;      /**< Image width [px] */
    int height;     /**< Image height [px] */
    float *pixels;  /**< Pixel data as grayscale floats */
} DetScatImage;

void detscat_camera_init(DetScatCamera *cam, DetScatCfg *cfg);

int detscat_camera_image_create(DetScatImage *image, int w, int h);

int detscat_camera_get_image_index(DetScatImage *img, int u, int v);

void detscat_camera_pixel_coordinate_to_world(const DetScatCamera *camera,
                                              Vec3 *p, int u, int v);

void detscat_camera_pixel_observation_direction(const DetScatCamera *camera,
                                                Vec3 *d, int u, int v);

#endif  // DETSCAT_CAMERA_H
