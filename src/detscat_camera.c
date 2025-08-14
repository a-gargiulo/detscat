#include "detscat_camera.h"

#include <assert.h>
#include <stdlib.h>


#include "detscat_config.h"
#include "detscat_const.h"
#include "mymath.h"

void detscat_camera_pixel_coordinate_to_world(const DetScatCamera *camera, Vec3 *p, int u, int v) {
    p->x = camera->p_x * (u - camera->c_x) * camera->r.x +
           camera->p_y * (v - camera->c_y) * camera->u.x +
           camera->f * camera->n.x +
           camera->C.x;
    p->y = camera->p_x * (u - camera->c_x) * camera->r.y +
           camera->p_y * (v - camera->c_y) * camera->u.y +
           camera->f * camera->n.y +
           camera->C.y;
    p->z = camera->p_x * (u - camera->c_x) * camera->r.z +
           camera->p_y * (v - camera->c_y) * camera->u.z +
           camera->f * camera->n.z +
           camera->C.z;
}


void detscat_camera_pixel_observation_direction(const DetScatCamera *camera, Vec3 *d, int u, int v) {
    Vec3 dir;

    dir.x = camera->p_x * (u - camera->c_x) * camera->r.x +
            camera->p_y * (v - camera->c_y) * camera->u.x +
            camera->f * camera->n.x;
    dir.y = camera->p_x * (u - camera->c_x) * camera->r.y +
            camera->p_y * (v - camera->c_y) * camera->u.y +
            camera->f * camera->n.y;
    dir.z = camera->p_x * (u - camera->c_x) * camera->r.z +
            camera->p_y * (v - camera->c_y) * camera->u.z +
            camera->f * camera->n.z;

    double dir_norm = mymath_vec3_abs(&dir);

    d->x = dir.x / dir_norm;
    d->y = dir.y / dir_norm;
    d->z = dir.z / dir_norm;
}


// TODO: Add robustness with checks for config variables and for norms
// TODO: Add a more generic / safe choice for yref
void detscat_camera_camera_create(DetScatCamera *camera, DetScatConfig *cfg) {
    assert(camera != NULL);
    assert(cfg != NULL);

    double tmp_norm;

    camera->C = cfg->camera_center_position_m;

    tmp_norm = mymath_vec3_abs(&cfg->camera_sensor_normal_vector);
    camera->n.x = cfg->camera_sensor_normal_vector.x / tmp_norm; 
    camera->n.y = cfg->camera_sensor_normal_vector.y / tmp_norm; 
    camera->n.z = cfg->camera_sensor_normal_vector.z / tmp_norm; 

    Vec3 yref = {0, -1, 0};
    Vec3 tmp_r;
    mymath_vec3_cross(&tmp_r, &yref, &camera->n);
    tmp_norm = mymath_vec3_abs(&tmp_r);
    camera->r.x = tmp_r.x / tmp_norm;
    camera->r.y = tmp_r.y / tmp_norm;
    camera->r.z = tmp_r.z / tmp_norm;

    Vec3 tmp_u;
    mymath_vec3_cross(&tmp_u, &camera->n, &camera->r);
    tmp_norm = mymath_vec3_abs(&tmp_u);
    camera->u.x = tmp_u.x / tmp_norm;
    camera->u.y = tmp_u.y / tmp_norm;
    camera->u.z = tmp_u.z / tmp_norm;


    camera->f = cfg->focal_length_mm * DETSCAT_CONST_MM2M;
    camera->p_x = cfg->sensor_width_mm * DETSCAT_CONST_MM2M / cfg->camera_resolution_x_px;
    camera->p_y = cfg->sensor_height_mm * DETSCAT_CONST_MM2M / cfg->camera_resolution_y_px;
    camera->width = cfg->camera_resolution_x_px;
    camera->height= cfg->camera_resolution_y_px;

    camera->c_x = (camera->width - 1.0) / 2.0;
    camera->c_y = (camera->height - 1.0) / 2.0;

    return;
}


int detscat_camera_image_create(DetScatImage *image, int w, int h)
{
    assert(image != NULL);

    if (w <= 0 || h <= 0) return -1;

    size_t size = (size_t)w * (size_t)h;
    // Check for overflow
    if (size / (size_t)w != (size_t)h) {
        return -2;
    }

    image->width = w;
    image->height = h;

    image->pixels = calloc(size, sizeof(float));
    if (!image->pixels) {
        return -3;
    }

    return 0;
}

int detscat_camera_get_image_index(DetScatImage* img, int u, int v) {
    // Row-major
    return v * img->width + u;
}
