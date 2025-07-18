#include "detscat_camera.h"

#include <stdlib.h>


#include "detscat_config.h"
#include "detscat_const.h"
#include "mymath.h"

void detscat_camera_pixel_coordinate_to_world(const Camera *camera, Vec3 *p, int u, int v) {
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


void detscat_camera_pixel_observation_direction(const Camera *camera, Vec3 *d, int u, int v) {
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
Camera *detscat_camera_create(DetScatConfig *cfg) {
    Camera *cam = malloc(sizeof(Camera));
    if (!cam) return NULL;

    double tmp_norm;

    cam->C = cfg->camera_center_position_m;

    tmp_norm = mymath_vec3_abs(&cfg->camera_sensor_normal_vector);
    cam->n.x = cfg->camera_sensor_normal_vector.x / tmp_norm; 
    cam->n.y = cfg->camera_sensor_normal_vector.y / tmp_norm; 
    cam->n.z = cfg->camera_sensor_normal_vector.z / tmp_norm; 

    Vec3 yref = {0, -1, 0};
    Vec3 tmp_r;
    mymath_vec3_cross(&tmp_r, &yref, &cam->n);
    tmp_norm = mymath_vec3_abs(&tmp_r);
    cam->r.x = tmp_r.x / tmp_norm;
    cam->r.y = tmp_r.y / tmp_norm;
    cam->r.z = tmp_r.z / tmp_norm;

    Vec3 tmp_u;
    mymath_vec3_cross(&tmp_u, &cam->n, &cam->r);
    tmp_norm = mymath_vec3_abs(&tmp_u);
    cam->u.x = tmp_u.x / tmp_norm;
    cam->u.y = tmp_u.y / tmp_norm;
    cam->u.z = tmp_u.z / tmp_norm;


    cam->f = cfg->focal_length_mm * DETSCAT_CONST_MM2M;
    cam->p_x = cfg->sensor_width_mm * DETSCAT_CONST_MM2M / cfg->camera_resolution_x_px;
    cam->p_y = cfg->sensor_height_mm * DETSCAT_CONST_MM2M / cfg->camera_resolution_y_px;
    cam->width = cfg->camera_resolution_x_px;
    cam->height= cfg->camera_resolution_y_px;

    cam->c_x = (cam->width - 1.0) / 2.0;
    cam->c_y = (cam->height - 1.0) / 2.0;

    return cam;
}


Image *detscat_camera_image_create(int w, int h)
{
    if (w <= 0 || h <= 0) return NULL;

    Image *img = malloc(sizeof(Image));
    if (!img) return NULL;

    size_t size = (size_t)w * (size_t)h;
    // Check for overflow
    if (size / (size_t)w != (size_t)h) {
        free(img);
        return NULL;
    }

    img->width = w;
    img->height = h;

    img->pixels = calloc(size, sizeof(float));

    if (!img->pixels) {
        free(img);
        return NULL;
    }

    return img;
}

int detscat_camera_get_image_index(Image* img, int u, int v) {
    // Row-major
    return v * img->width + u;
}
