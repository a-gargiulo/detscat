#include "detscat.h"

#include "detscat_log.h"
#include "detscat_cfg.h"
#include "detscat_prt.h"

#include <stdbool.h>
#include <stdio.h>

static void detscat_print_banner(FILE *stream_out) {
    if (!stream_out) stream_out = stdout;

    fprintf(stream_out,
        "****************************************\n"
        "  _____       _    _____           _    \n"
        " |  __ \\     | |  / ____|         | |   \n"
        " | |  | | ___| |_| (___   ___ __ _| |_  \n"
        " | |  | |/ _ \\ __|\\___ \\ / __/ _` | __| \n"
        " | |__| |  __/ |_ ____) | (_| (_| | |_  \n"
        " |_____/ \\___|\\__|_____/ \\___\\__,_|\\__| \n"
        "\n"
        "****************************************\n"
        "\n"
        "Version: 1.0.0\n"
        "Build date: %s, %s\n"
        "Compiler: %s\n"
        "Author: Aldo Gargiulo\n"
        "\n"
        "\n",
        __DATE__, __TIME__, __VERSION__);
}

bool detscat_init(void) {
    detscat_log_init_lock();
    detscat_print_banner(NULL);
    return true;
}

bool detscat_terminate(void) {
    detscat_log_terminate_lock();
    return true;
}

void detscat_data_free(DetScat *detscat) {
    if (!detscat) return;

    detscat_cfg_destroy(&detscat->cfg);
}


bool detscat_load_data(const char *config_path, DetScat* detscat, DetScatDiagnose* diag) {

    if (!detscat_cfg_create(&detscat->cfg, diag)) return false;

    if (!detscat_cfg_load(config_path, detscat->cfg, diag)) goto cleanup_cfg;

    if (!detscat_prt_create(&detscat->prt, diag)) goto cleanup_cfg;

    const char *particles_path = detscat->cfg->particles_file_path.data;
    if (!detscat_prt_load(particles_path, detscat->prt, diag)) goto cleanup_prt;




    goto success;
cleanup_prt:
    detscat_prt_destroy(&detscat->prt);
cleanup_cfg:
    detscat_cfg_destroy(&detscat->cfg);
    return false;
success:
    return true;
}

static void vec3_print(const Vec3 *v) {
    printf("[%10.6f, %10.6f, %10.6f ]", v->x, v->y, v->z);
}

static void complexvec3_print(const ComplexVec3 *v) {
    printf("[%.6f+%.6fi, %.6f+%.6fi, %.6f+%.6fi]",
           v->x.re, v->x.im,
           v->y.re, v->y.im,
           v->z.re, v->z.im);
}

static void str_print(const Str *s) {
    printf("\"%s\"", s->data);
}

void detscat_print_cfg(const DetScatConfig *cfg) {
    if (!cfg) {
        printf("<null config>\n");
        return;
    }


    printf("\n");

   // For strings, use         %-25s to left-align within 25 chars
    printf("        %-25s: \"%s\"\n", "particles_file_path", cfg->particles_file_path.data);

    // For ComplexVec3, align the label
    printf("        %-25s: ", "polarization"); complexvec3_print(&cfg->polarization); printf("\n");

    // Align doubles
    printf("        %-25s: %10.6f\n", "wavelength_nm", cfg->wavelength_nm);
    printf("        %-25s: %10.6f\n", "pulse_energy_mj", cfg->pulse_energy_mj);
    printf("        %-25s: %10.6f\n", "pulse_width_ns", cfg->pulse_width_ns);
    printf("        %-25s: %10.6f\n", "beam_diameter_mm", cfg->beam_diameter_mm);

    // Bool as string
    printf("        %-25s: %s\n", "is_polarized", cfg->is_polarized ? "true" : "false");

    // Vec3
    printf("        %-25s: ", "camera_center_position_m"); vec3_print(&cfg->camera_center_position_m); printf("\n");
    printf("        %-25s: ", "camera_sensor_normal"); vec3_print(&cfg->camera_sensor_normal); printf("\n");

    // More doubles
    printf("        %-25s: %10.6f\n", "focal_length_mm", cfg->focal_length_mm);
    printf("        %-25s: %10.6f\n", "sensor_width_mm", cfg->sensor_width_mm);
    printf("        %-25s: %10.6f\n", "sensor_height_mm", cfg->sensor_height_mm);

    // Integers
    printf("        %-25s: %6d\n", "camera_resolution_x_px", cfg->camera_resolution_x_px);
    printf("        %-25s: %6d\n", "camera_resolution_y_px", cfg->camera_resolution_y_px);

    printf("\n");



}
 




//////////////////////////////
// // #include <assert.h>
// // #include <stdio.h>

// // #include <errno.h>
// // #include <math.h>
// // #include <omp.h>
// // #include <stdarg.h>
// // #include <stdlib.h>
// // #include <string.h>

// // #include "detscat_camera.h"
// // #include "detscat_config.h"
// // #include "detscat_log.h"
// // #include "detscat_particles.h"
// // #include "detscat_const.h"
// // #include "detscat_ddscat.h"


// static void detscat_print_banner(void) {
//     printf(
//         "****************************************\n"
//         "  _____       _    _____           _    \n"
//         " |  __ \\     | |  / ____|         | |   \n"
//         " | |  | | ___| |_| (___   ___ __ _| |_  \n"
//         " | |  | |/ _ \\ __|\\___ \\ / __/ _` | __| \n"
//         " | |__| |  __/ |_ ____) | (_| (_| | |_  \n"
//         " |_____/ \\___|\\__|_____/ \\___\\__,_|\\__| \n"
//         "                                        \n"
//         "                                        \n"
//         "(C) Aldo Gargiulo 2025                  \n"
//         "                                        \n"
//         "****************************************\n");
//     return;
// }

// // static int cmp_phi(const void *a, const void *b) {
// //     const DetScatDdscatFmatrix *fa = (const DetScatDdscatFmatrix *)a;
// //     const DetScatDdscatFmatrix *fb = (const DetScatDdscatFmatrix *)b;
// //     return (fa->phi > fb->phi) - (fa->phi < fb->phi);
// // }

// // static bool find_type_index(const DetScatPrtData *prt, const char *type_id, size_t *out_idx) {
// //     for (size_t j = 0; j < prt->n_types; ++j) {
// //         if (strcmp(type_id, prt->types[j].type_id) == 0) {
// //             *out_idx = j;
// //             return true;
// //         }
// //     }
// //     return false;
// // }

// // static void detscat_ddscat_load(DetScatDdscatData *ddscat,
// //                                 DetScatPrtData *prt,
// //                                 DetScatDiagnose *diagnose) {
// //     assert(ddscat != NULL);
// //     assert(prt != NULL);
// //     assert(prt->n_particles > 0);
// //     assert(prt->n_types > 0);
// //     assert(diagnose != NULL);

// //     diagnose->status = DETSCAT_OK;

// //     bool parser_init = false;
// //     DetScatDdscatParser parser = {0};

// //     if (!detscat_ddscat_init(ddscat, prt->n_types, prt->n_particles, prt->n_particles)) {
// //         detscat_prt_free(prt);
// //         DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_ALLOC,
// //                              "%s",
// //                              "Initialization of DDSCAT data failed.");
// //         return;
// //     }

// //     // PAR
// //     for (size_t i = 0; i < prt->n_types; ++i) {
// //         char par_file_path[DETSCAT_PATH_MAX];

// //         const char *par_dir = prt->types[i].data_dir;
// //         const char *tid = prt->types[i].type_id;

// //         size_t len = strlen(par_dir);
// //         size_t extra = DETSCAT_DDSCAT_PAR_FILENAME_LEN;  // ddscat.par - 10 bytes (characters)
// //         if (len > 0 && par_dir[len - 1] != '/') extra++;  // add 1 for '/'

// //         if (len + extra > DETSCAT_PATH_MAX - 1) {
// //             DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
// //                                  "%s",
// //                                  "Path to .par file is too large");
// //             return;
// //         }

// //         snprintf(par_file_path, DETSCAT_PATH_MAX, 
// //                  "%s%s%s",
// //                  par_dir,
// //                  (len > 0 && par_dir[len - 1] != '/') ? "/" : "",
// //                  "ddscat.par");

// //         errno = 0;
// //         bool success;
// //         const char *errmsg = NULL;

// //         if (!parser_init) {
// //             success = detscat_ddscat_parser_init(&parser, par_file_path); 
// //             errmsg = "Could not initialize ddscat parser from";
// //         } 
// //         else {
// //             success = detscat_ddscat_parser_reset(&parser, par_file_path); 
// //             errmsg = "Could not reset ddscat parser from";
// //         }

// //         if (!success) {
// //             DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
// //                                  "%s '%s': %s",
// //                                  errmsg,
// //                                  par_file_path,
// //                                  strerror(errno));
// //             if (parser_init) detscat_ddscat_parser_close(&parser);
// //             return;
// //         }

// //         parser_init = true;

// //         if (!detscat_ddscat_parser_par_load(&parser, &ddscat->pars[i])) {
// //             DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
// //                                  "Could not parse the DDSCAT parameter file "
// //                                  "'%s' for particle type "
// //                                  "'%s'.",
// //                                  par_file_path,
// //                                  tid);
// //             detscat_ddscat_parser_close(&parser);
// //             return;
// //         }
// //     }

// //     // FML
// //     for (size_t i = 0; i < prt->n_particles; ++i) {
// //         size_t idx;
// //         if (!find_type_index(prt, prt->particles[i].type_id, &idx)) {
// //             DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_LOOKUP,
// //                                  "Particle type '%s' not found in definitions.",
// //                                  prt->particles[i].type_id);
// //             detscat_ddscat_parser_close(&parser);
// //             return;
// //         }
// //         ddscat->par_idxs[i] = idx;

// //         const char* fml_dir = prt->types[idx].data_dir;
// //         size_t len = strlen(fml_dir);
// //         size_t extra = DETSCAT_DDSCAT_FML_FILENAME_LEN;  // wxxxryyykzzz.fml - 16 bytes (characters)

// //         char fml_file_path[DETSCAT_PATH_MAX];

// //         if (len > 0 && fml_dir[len - 1] != '/') extra++;  // add 1 for '/'

// //         if (len + extra > DETSCAT_PATH_MAX - 1) {
// //             DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
// //                                  "%s",
// //                                  "Path to .fml file is too large");
// //             detscat_ddscat_parser_close(&parser);
// //             return;
// //         }

// //         snprintf(fml_file_path, DETSCAT_PATH_MAX, 
// //                  "%s%sw%03dr%03dk%03d.fml",
// //                  fml_dir,
// //                  (len > 0 && fml_dir[len - 1] != '/') ? "/" : "",
// //                  prt->particles[i].case_id.w,
// //                  prt->particles[i].case_id.r,
// //                  prt->particles[i].case_id.k);

// //         errno = 0;
// //         if (!detscat_ddscat_parser_reset(&parser, fml_file_path)) {
// //             DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
// //                                  "Could not reset ddscat parser from '%s': %s",
// //                                  fml_file_path,
// //                                  strerror(errno));
// //             detscat_ddscat_parser_close(&parser);
// //             return;
// //         }

// //         if (!detscat_ddscat_parser_fml_load(&parser, &ddscat->fmls[i], &ddscat->pars[idx])) {
// //             DETSCAT_SET_DIAGNOSE(
// //                 *diagnose, DETSCAT_ERR_PARSING,
// //                 "Could not parse the DDSCAT fml file '%s' for particle number "
// //                 "'%zu'.",
// //                 fml_file_path, i + 1);
// //             detscat_ddscat_parser_close(&parser);
// //             return;
// //         }
// //     }

// //     detscat_ddscat_parser_close(&parser);
// //     detscat_info("Successfully fetched all DDSCAT data.");
// //     return;
// // }

// // static void detscat_get_camera_and_image(DetScatCamera *camera,
// //                                          DetScatImage *image,
// //                                          DetScatConfig *config,
// //                                          DetScatDiagnose *diagnose) {
// //     assert(camera != NULL);
// //     assert(image != NULL);
// //     assert(config != NULL);
// //     assert(diagnose != NULL);

// //     detscat_camera_init(camera, config);
// //     int img_gen_status = detscat_camera_image_create(image, camera->width, camera->height); 
// //     if (img_gen_status != 0) {
// //         switch (img_gen_status) {
// //             case -1:
// //                 DETSCAT_SET_DIAGNOSE(
// //                     *diagnose, DETSCAT_ERR_INVALID_ARG,
// //                     "Specified image size %d x %d is invalid.",
// //                     camera->width, camera->height);
// //                 return;
// //             case -2:
// //                 DETSCAT_SET_DIAGNOSE(
// //                     *diagnose, DETSCAT_ERR_OVERFLOW,
// //                     "%s",
// //                     "Image size too large - overflow.");
// //                 return;
// //             case -3:
// //                 DETSCAT_SET_DIAGNOSE(
// //                     *diagnose, DETSCAT_ERR_ALLOC,
// //                     "%s",
// //                     "Image allocation failed.");
// //                 return;
// //         }
// //     }

// //     return;
// // } 

// DetScatStatus detscat_run(int argc, char **argv) {
//     detscat_print_banner();

//     DetScatDiagnose *diag = detscat_diag_create();
//     if (!diag) {
//         DETSCAT_LOG_ERROR_NO_DIAGNOSE(
//             DETSCAT_ERR_MEMORY, "%s",
//             "Failed to allocate memory for diagnostics");
//         return DETSCAT_ERR_MEMORY;
//     }

//     if (argc < 2 || argv[1][0] == '\0') {
//         DETSCAT_SET_DIAGNOSE(
//             diag, DETSCAT_ERR_CMD_ARGUMENT,
//             "Provide path to configuration file: "
//             "%s <path_to_configuration_file>",
//             argv[0]);
//         goto cleanup_cmd;
//     }
//     detscat_log_info("DetScat initialized successfully");

//     // DetScatCfg cfg;
//     // if (!detscat_cfg_init(&cfg, diag)) goto cleanup_cfg;

//     // DetScatPrt prt = {0};

    
//     // DetScatDdscatData ddscat = {0};

//     // DetScatCamera camera = {0};
//     // DetScatImage image = {0};

//     // const char *cfg_file_path = argv[1];
//     // if (!detscat_cfg_load(cfg_file_path, &cfg, diag)) goto cleanup_cfg;

//     // if (!detscat_prt_load(cfg.particles_file.data, &prt, diag)) goto cleanup_prt;
//     // detscat_prt_load(cfg.particles_file, &prt, diagnose);
//     // if (diagnose->status != DETSCAT_OK) return;

//     // detscat_ddscat_load(&ddscat, &prt, diagnose);
//     // if (diagnose->status != DETSCAT_OK) goto cleanup_ddscat;

//     // detscat_get_camera_and_image(&camera, &image, &config, diagnose);
//     // if (diagnose->status != DETSCAT_OK) return;

//     // MAIN LOOP
//     // #pragma omp parallel for collapse(2)
//     // for (int u = 0; u < image->width; ++u) {
//     //     for (int v = 0; v < image->height; ++v) {
//     //         int pxl_idx = detscat_camera_get_image_index(image, u, v);
//     //         Vec3 x_pxl_w;
//     //         detscat_camera_pixel_coordinate_to_world(camera, &x_pxl_w, u, v);

//     //         for (size_t p = 0; p < particles_data.n_particles; ++p) {
//     //             Vec3 x_s_w;
//     //             mymath_vec3_sub(&x_s_w, &x_pxl_w,
//     //             &particles_data.particles[p].position); double x_s_w_abs =
//     //             mymath_vec3_abs(&x_s_w);

//     //             Vec3 d_s;
//     //             mymath_vec3_normalize(&d_s, &x_s_w, x_s_w_abs);
//     //             double k = 2.0 * M_PI / (config.wavelength_nm *
//     //             DETSCAT_CONST_NM2M); Vec3 k_s = {k * d_s.x, k * d_s.y, k *
//     //             d_s.z};

//     //             double phi = atan2(k_s.z, k_s.y) * 180.0 / M_PI;
//     //             double theta = acos(k_s.x / k) * 180.0 / M_PI;

//     //             size_t nplanes = par[fmat_to_par_map[p]]->nplanes;
//     //             // sort phi
//     //             qsort(fmat[p], nplanes, sizeof(Fmat), cmp_phi);
//     //             size_t idx_low = 0;
//     //             size_t idx_high = 0;
//     //             for (size_t m = 0; m < nplanes - 1; ++m) {
//     //                 double phi_low = fmat[p][m].phi;
//     //                 double phi_high = fmat[p][m + 1].phi;
//     //                 if (phi >= phi_low && phi < phi_high) {
//     //                     idx_low = m;
//     //                     idx_high = m + 1;
//     //                     break;
//     //                 }
//     //             }
//     //             if (idx_low == idx_high) {
//     //                 // TODO: HANDLE ERROR
//     //                 return;
//     //             }

//     //             // Interpolate between fmat[p][idx_low] and fmat[p][idx_high]
//     //             for each theta
//     //         }
//         // }
//     // }

//     // detscat_prt_free(&prt);
//     // detscat_ddscat_free(&ddscat);
//     goto success;
    
// // cleanup_ddscat:
// //     detscat_prt_free(&prt);
// //     detscat_ddscat_free(&ddscat);
// //     return;
// // cleanup_prt:
// //     detscat_prt_free(&prt);
// // cleanup_cfg:
// //     detscat_cfg_free(&cfg);
// //     return diag->status;
// cleanup_cmd:
//     detscat_log_error_diagnose(diag);
//     return detscat_diag_status(diag);
// success:
//     // detscat_cfg_free(&cfg);
//     // detscat_prt_free(&prt);
//     detscat_diag_free(diag);
//     diag = NULL;
//     return DETSCAT_OK;
// }

// // double detscat_calculate_incident_field_strength(double d, double E_p_mj,
// // double tau_p_ns) {
// //     double A = d * d * DETSCAT_CONST_MM2M * DETSCAT_CONST_MM2M * M_PI / 4.0;
// //     double I = E_p_mj * DETSCAT_CONST_MJ2J / tau_p_ns / DETSCAT_CONST_NS2S /
// //     A; return sqrt(2 * I / DETSCAT_CONST_C_MS / DETSCAT_CONST_EPS0_F_M);
// // }
