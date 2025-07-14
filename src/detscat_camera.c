#include "detscat_camera.h"

#include <stdlib.h>


#include "detscat_config.h"
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

    double dir_norm = mymath_vec3_norm(&dir);

    d->x = dir.x / dir_norm;
    d->y = dir.y / dir_norm;
    d->z = dir.z / dir_norm;
}


Camera *detscat_camera_create(DetScatConfig *cfg) {
    Camera *cam = malloc(sizeof(Camera));
    if (!cam) return NULL;

    cam->C = cfg->camera


    return cam;
}
