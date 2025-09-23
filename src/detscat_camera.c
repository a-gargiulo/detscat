#include "detscat_camera.h"

#include "detscat.h"

#include "detscat_cfg.h"
#include "detscat_error.h"
#include "detscat_const.h"
#include "detscat_transform.h"
#include "detscat_math.h"


#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

// --- Public API ---
bool detscat_camera_init(DetScatCamera *cam, const DetScatConfig *cfg, const DetScatTransform *glob_t, DetScatError *err) {
    assert(cam && cfg);


    // Forward (view direction) in CALC frame
    detscat_math_mat3_vec3_mult(&cam->axes.n, &glob_t->rotation, &cfg->camera_direction);
    detscat_math_vec3_normalize(&cam->axes.n, &cam->axes.n, detscat_math_vec3_abs(&cam->axes.n));


    // Build right and up axes from "preferred up" direction 
    Vec3 preferred_up_global = (Vec3){0, -1, 0};
    Vec3 preferred_up_calc;
    detscat_math_mat3_vec3_mult(&preferred_up_calc, &glob_t->rotation, &preferred_up_global);
    
    detscat_math_vec3_build_basis(&cam->axes.n, &preferred_up_calc, AXIS_CAMERA, &cam->axes.r, &cam->axes.u, &cam->axes.n);

    // Rotation matrix CALC -> CAMERA
    detscat_math_mat3_basis_to_rotmat(&cam->extrinsics.rotation, &cam->axes.r, &cam->axes.u, &cam->axes.n);

    // Translation vector - Camera center in CAMERA frame 
    Vec3 cam_cntr_rot1;
    Vec3 cam_cntr_rot2;
    Vec3 cam_cntr_t1;
    // camera center expressed in CALC frame
    detscat_math_mat3_vec3_mult(&cam_cntr_rot1, &glob_t->rotation, &cfg->camera_center_position_m); 
    detscat_math_vec3_add(&cam_cntr_t1, &cam_cntr_rot1, &glob_t->translation);
    // find translation in CAMERA frame
    detscat_math_mat3_vec3_mult(&cam_cntr_rot2, &cam->extrinsics.rotation, &cam_cntr_rot1);
    detscat_math_vec3_scale(&cam->extrinsics.translation, &cam_cntr_rot2, -1);

    // Camera intrinsics
    cam->intrinsics.f = cfg->focal_length_mm * DETSCAT_CONST_MM2M;
    cam->intrinsics.p_x = cfg->sensor_width_mm * DETSCAT_CONST_MM2M / cfg->sensor_resolution_x_px;
    cam->intrinsics.p_y = cfg->sensor_height_mm * DETSCAT_CONST_MM2M / cfg->sensor_resolution_y_px;

    cam->intrinsics.c_x = (cfg->sensor_resolution_x_px - 1.0) / 2.0;
    cam->intrinsics.c_y = (cfg->sensor_resolution_y_px - 1.0) / 2.0;

    cam->image.width = cfg->sensor_resolution_x_px;
    cam->image.height= cfg->sensor_resolution_y_px;

    size_t size = (size_t)(cam->image.width) * (size_t)(cam->image.height);
    // Check for overflow
    if (size / (size_t)(cam->image.width) != (size_t)(cam->image.height)) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_OVERFLOW,
                          "Image size computation overflow");
        return false;
    }

    cam->image.pixels = calloc(size, sizeof(unsigned char));
    if (!cam->image.pixels) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Could not allocate memory for image");
        return false;
    }

    cam->image.intensities = calloc(size, sizeof(double));
    if (!cam->image.intensities) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Could not allocate memory for intensities");
        return false;
    }

    return true;
}

void detscat_camera_free(DetScatCamera *cam) {
    if (!cam) return;

    if (cam->image.pixels) {
        free(cam->image.pixels);
        cam->image.pixels = NULL;
    }

    if (cam->image.intensities) {
        free(cam->image.intensities);
        cam->image.intensities = NULL;
    }

    cam->axes = (DetScatCameraAxes){0};
    cam->intrinsics = (DetScatCameraIntrinsic){0};
    cam->extrinsics = (DetScatCameraExtrinsic){0};
    cam->image.width = 0;
    cam->image.height = 0;
}


void detscat_camera_pixel_coordinate_to_world(Vec3 *p, int u, int v, const DetScatCamera *cam) {
    // p->x = cam->intrinsics.p_x * (u - cam->intrinsics.c_x) * cam->extrinsics.rotation.m11 +
    //        cam->intrinsics.p_y * (v - cam->intrinsics.c_y) * cam->extrinsics.rotation.m21 +
    //        cam->intrinsics.f * cam->extrinsics.rotation.m31 -
    //        cam->extrinsics.translation.x;
    // p->y = cam->intrinsics.p_x * (u - cam->intrinsics.c_x) * cam->extrinsics.rotation.m12 +
    //        cam->intrinsics.p_y * (v - cam->intrinsics.c_y) * cam->extrinsics.rotation.m22 +
    //        cam->intrinsics.f * cam->extrinsics.rotation.m32 -
    //        cam->extrinsics.translation.y;
    // p->z = cam->intrinsics.p_x * (u - cam->intrinsics.c_x) * cam->extrinsics.rotation.m13 +
    //        cam->intrinsics.p_y * (v - cam->intrinsics.c_y) * cam->extrinsics.rotation.m23 +
    //        cam->intrinsics.f * cam->extrinsics.rotation.m33 -
    //        cam->extrinsics.translation.z;
    p->x = cam->intrinsics.p_x * (u - cam->intrinsics.c_x) * cam->extrinsics.rotation.m11 +
           cam->intrinsics.p_y * (v - cam->intrinsics.c_y) * cam->extrinsics.rotation.m21 +
           cam->intrinsics.f * cam->extrinsics.rotation.m31 -
           cam->extrinsics.translation.x * cam->extrinsics.rotation.m11 -
           cam->extrinsics.translation.y * cam->extrinsics.rotation.m21 -
           cam->extrinsics.translation.z * cam->extrinsics.rotation.m31;
    p->y = cam->intrinsics.p_x * (u - cam->intrinsics.c_x) * cam->extrinsics.rotation.m12 +
           cam->intrinsics.p_y * (v - cam->intrinsics.c_y) * cam->extrinsics.rotation.m22 +
           cam->intrinsics.f * cam->extrinsics.rotation.m32 -
           cam->extrinsics.translation.x * cam->extrinsics.rotation.m12 -
           cam->extrinsics.translation.y * cam->extrinsics.rotation.m22 -
           cam->extrinsics.translation.z * cam->extrinsics.rotation.m32;
    p->z = cam->intrinsics.p_x * (u - cam->intrinsics.c_x) * cam->extrinsics.rotation.m13 +
           cam->intrinsics.p_y * (v - cam->intrinsics.c_y) * cam->extrinsics.rotation.m23 +
           cam->intrinsics.f * cam->extrinsics.rotation.m33 -
           cam->extrinsics.translation.x * cam->extrinsics.rotation.m13 -
           cam->extrinsics.translation.y * cam->extrinsics.rotation.m23 -
           cam->extrinsics.translation.z * cam->extrinsics.rotation.m33;
}


// void detscat_camera_pixel_observation_direction(const DetScatCamera *camera, Vec3 *d, int u, int v) {
//     Vec3 dir;

//     dir.x = camera->p_x * (u - camera->c_x) * camera->r.x +
//             camera->p_y * (v - camera->c_y) * camera->u.x +
//             camera->f * camera->n.x;
//     dir.y = camera->p_x * (u - camera->c_x) * camera->r.y +
//             camera->p_y * (v - camera->c_y) * camera->u.y +
//             camera->f * camera->n.y;
//     dir.z = camera->p_x * (u - camera->c_x) * camera->r.z +
//             camera->p_y * (v - camera->c_y) * camera->u.z +
//             camera->f * camera->n.z;

//     double dir_norm = mymath_vec3_abs(&dir);

//     d->x = dir.x / dir_norm;
//     d->y = dir.y / dir_norm;
//     d->z = dir.z / dir_norm;
// }


int detscat_camera_get_pixel_index(int u, int v, const DetScatImage* img) {
    // Row-major
    return v * img->width + u;
}
