#include "detscat.h"

#include <assert.h>
#include <errno.h>
// #include <math.h>
#include <omp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "detscat_camera.h"
#include "detscat_config.h"
// #include "detscat_const.h"
#include "detscat_ddscat.h"
#include "detscat_particles.h"

static void detscat_print_banner(void) {
    printf(
        "****************************************\n"
        "  _____       _    _____           _    \n"
        " |  __ \\     | |  / ____|         | |   \n"
        " | |  | | ___| |_| (___   ___ __ _| |_  \n"
        " | |  | |/ _ \\ __|\\___ \\ / __/ _` | __| \n"
        " | |__| |  __/ |_ ____) | (_| (_| | |_  \n"
        " |_____/ \\___|\\__|_____/ \\___\\__,_|\\__| \n"
        "                                        \n"
        "                                        \n"
        "(C) Aldo Gargiulo 2025                  \n"
        "                                        \n"
        "****************************************\n");
    return;
}


static void detscat_cfg_load(const char *file_path,
                             DetScatCfg *cfg,
                             DetScatDiagnose *diagnose) {
    assert(file_path != NULL && file_path[0] != '\0');
    assert(cfg != NULL);
    assert(diagnose != NULL);

    diagnose->status = DETSCAT_OK;

    DetScatCfgParser parser = {0};
    errno = 0;
    if (!detscat_cfg_parser_init(&parser, file_path)) {
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
                             "Could not open '%s': %s",
                             file_path, strerror(errno));
        return;
    }

    if (!detscat_cfg_parser_load(&parser, cfg)) {
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
                             "While parsing '%s': %s",
                             file_path, parser.errmsg);
        detscat_cfg_parser_close(&parser);
        return;
    }
    detscat_cfg_parser_close(&parser);

    detscat_info("Successfully parsed '%s'", file_path);
    return;
}

static void detscat_prt_load(const char* file_path,
                             DetScatPrtData *prt,
                             DetScatDiagnose *diagnose) {
    assert(file_path != NULL && file_path[0] != '\0');
    assert(prt != NULL);
    assert(diagnose != NULL);

    diagnose->status = DETSCAT_OK;

    DetScatPrtParser parser = {0};
    errno = 0;
    if (!detscat_prt_parser_init(&parser, file_path)) {
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
                             "Could not open '%s': %s",
                             file_path, strerror(errno));
        return;
    }

    if (!detscat_prt_parser_load(&parser, prt)) {
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
                             "While parsing '%s': %s",
                             file_path, parser.errmsg);
        detscat_prt_parser_close(&parser);
        return;
    }
    detscat_prt_parser_close(&parser);

    detscat_info("Successfully parsed '%s'.", file_path);
    return;
}

static int cmp_phi(const void *a, const void *b) {
    const DetScatDdscatFmatrix *fa = (const DetScatDdscatFmatrix *)a;
    const DetScatDdscatFmatrix *fb = (const DetScatDdscatFmatrix *)b;
    return (fa->phi > fb->phi) - (fa->phi < fb->phi);
}

static bool find_type_index(const DetScatPrtData *prt, const char *type_id, size_t *out_idx) {
    for (size_t j = 0; j < prt->n_types; ++j) {
        if (strcmp(type_id, prt->types[j].type_id) == 0) {
            *out_idx = j;
            return true;
        }
    }
    return false;
}

static void detscat_ddscat_load(DetScatDdscatData *ddscat,
                                DetScatPrtData *prt,
                                DetScatDiagnose *diagnose) {
    assert(ddscat != NULL);
    assert(prt != NULL);
    assert(prt->n_particles > 0);
    assert(prt->n_types > 0);
    assert(diagnose != NULL);

    diagnose->status = DETSCAT_OK;

    bool parser_init = false;
    DetScatDdscatParser parser = {0};

    if (!detscat_ddscat_init(ddscat, prt->n_types, prt->n_particles, prt->n_particles)) {
        detscat_prt_free(prt);
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_ALLOC,
                             "%s",
                             "Initialization of DDSCAT data failed.");
        return;
    }

    // PAR
    for (size_t i = 0; i < prt->n_types; ++i) {
        char par_file_path[DETSCAT_PATH_MAX];

        const char *par_dir = prt->types[i].data_dir;
        const char *tid = prt->types[i].type_id;

        size_t len = strlen(par_dir);
        size_t extra = DETSCAT_DDSCAT_PAR_FILENAME_LEN;  // ddscat.par - 10 bytes (characters)
        if (len > 0 && par_dir[len - 1] != '/') extra++;  // add 1 for '/'

        if (len + extra > DETSCAT_PATH_MAX - 1) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
                                 "%s",
                                 "Path to .par file is too large");
            return;
        }

        snprintf(par_file_path, DETSCAT_PATH_MAX, 
                 "%s%s%s",
                 par_dir,
                 (len > 0 && par_dir[len - 1] != '/') ? "/" : "",
                 "ddscat.par");

        errno = 0;
        bool success;
        const char *errmsg = NULL;

        if (!parser_init) {
            success = detscat_ddscat_parser_init(&parser, par_file_path); 
            errmsg = "Could not initialize ddscat parser from";
        } 
        else {
            success = detscat_ddscat_parser_reset(&parser, par_file_path); 
            errmsg = "Could not reset ddscat parser from";
        }

        if (!success) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
                                 "%s '%s': %s",
                                 errmsg,
                                 par_file_path,
                                 strerror(errno));
            if (parser_init) detscat_ddscat_parser_close(&parser);
            return;
        }

        parser_init = true;

        if (!detscat_ddscat_parser_par_load(&parser, &ddscat->pars[i])) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
                                 "Could not parse the DDSCAT parameter file "
                                 "'%s' for particle type "
                                 "'%s'.",
                                 par_file_path,
                                 tid);
            detscat_ddscat_parser_close(&parser);
            return;
        }
    }

    // FML
    for (size_t i = 0; i < prt->n_particles; ++i) {
        size_t idx;
        if (!find_type_index(prt, prt->particles[i].type_id, &idx)) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_LOOKUP,
                                 "Particle type '%s' not found in definitions.",
                                 prt->particles[i].type_id);
            detscat_ddscat_parser_close(&parser);
            return;
        }
        ddscat->par_idxs[i] = idx;

        const char* fml_dir = prt->types[idx].data_dir;
        size_t len = strlen(fml_dir);
        size_t extra = DETSCAT_DDSCAT_FML_FILENAME_LEN;  // wxxxryyykzzz.fml - 16 bytes (characters)

        char fml_file_path[DETSCAT_PATH_MAX];

        if (len > 0 && fml_dir[len - 1] != '/') extra++;  // add 1 for '/'

        if (len + extra > DETSCAT_PATH_MAX - 1) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
                                 "%s",
                                 "Path to .fml file is too large");
            detscat_ddscat_parser_close(&parser);
            return;
        }

        snprintf(fml_file_path, DETSCAT_PATH_MAX, 
                 "%s%sw%03dr%03dk%03d.fml",
                 fml_dir,
                 (len > 0 && fml_dir[len - 1] != '/') ? "/" : "",
                 prt->particles[i].case_id.w,
                 prt->particles[i].case_id.r,
                 prt->particles[i].case_id.k);

        errno = 0;
        if (!detscat_ddscat_parser_reset(&parser, fml_file_path)) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_PARSING,
                                 "Could not reset ddscat parser from '%s': %s",
                                 fml_file_path,
                                 strerror(errno));
            detscat_ddscat_parser_close(&parser);
            return;
        }

        if (!detscat_ddscat_parser_fml_load(&parser, &ddscat->fmls[i], &ddscat->pars[idx])) {
            DETSCAT_SET_DIAGNOSE(
                *diagnose, DETSCAT_ERR_PARSING,
                "Could not parse the DDSCAT fml file '%s' for particle number "
                "'%zu'.",
                fml_file_path, i + 1);
            detscat_ddscat_parser_close(&parser);
            return;
        }
    }

    detscat_ddscat_parser_close(&parser);
    detscat_info("Successfully fetched all DDSCAT data.");
    return;
}

static void detscat_get_camera_and_image(DetScatCamera *camera,
                                         DetScatImage *image,
                                         DetScatConfig *config,
                                         DetScatDiagnose *diagnose) {
    assert(camera != NULL);
    assert(image != NULL);
    assert(config != NULL);
    assert(diagnose != NULL);

    detscat_camera_init(camera, config);
    int img_gen_status = detscat_camera_image_create(image, camera->width, camera->height); 
    if (img_gen_status != 0) {
        switch (img_gen_status) {
            case -1:
                DETSCAT_SET_DIAGNOSE(
                    *diagnose, DETSCAT_ERR_INVALID_ARG,
                    "Specified image size %d x %d is invalid.",
                    camera->width, camera->height);
                return;
            case -2:
                DETSCAT_SET_DIAGNOSE(
                    *diagnose, DETSCAT_ERR_OVERFLOW,
                    "%s",
                    "Image size too large - overflow.");
                return;
            case -3:
                DETSCAT_SET_DIAGNOSE(
                    *diagnose, DETSCAT_ERR_ALLOC,
                    "%s",
                    "Image allocation failed.");
                return;
        }
    }

    return;
} 

int detscat_run(int argc, char **argv, DetScatDiagnose *diagnose) {
    assert(diagnose != NULL);

    detscat_print_banner();

    if (argc < 2 || argv[1][0] == '\0') {
        DETSCAT_SET_DIAGNOSE(
            *diagnose, DETSCAT_ERR_MISSING_CMD_ARG,
            "Missing command-line argument. Specify configuration file. Usage: "
            "%s <path_to_configuration_file>",
            argv[0]);
        return;
    }
    detscat_info("DetScat initialized successfully");

    DetScatCfg cfg = {0};
    DetScatPrtData prt = {0};
    DetScatDdscatData ddscat = {0};

    DetScatCamera camera = {0};
    DetScatImage image = {0};


    const char *cfg_file_path = argv[1];
    detscat_cfg_load(cfg_file_path, &cfg, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

    detscat_prt_load(cfg.particles_file, &prt, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

    detscat_ddscat_load(&ddscat, &prt, diagnose);
    if (diagnose->status != DETSCAT_OK) goto cleanup_ddscat;

    detscat_get_camera_and_image(&camera, &image, &config, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

    // MAIN LOOP
    #pragma omp parallel for collapse(2)
    for (int u = 0; u < image->width; ++u) {
        for (int v = 0; v < image->height; ++v) {
    //         int pxl_idx = detscat_camera_get_image_index(image, u, v);
    //         Vec3 x_pxl_w;
    //         detscat_camera_pixel_coordinate_to_world(camera, &x_pxl_w, u, v);

    //         for (size_t p = 0; p < particles_data.n_particles; ++p) {
    //             Vec3 x_s_w;
    //             mymath_vec3_sub(&x_s_w, &x_pxl_w,
    //             &particles_data.particles[p].position); double x_s_w_abs =
    //             mymath_vec3_abs(&x_s_w);

    //             Vec3 d_s;
    //             mymath_vec3_normalize(&d_s, &x_s_w, x_s_w_abs);
    //             double k = 2.0 * M_PI / (config.wavelength_nm *
    //             DETSCAT_CONST_NM2M); Vec3 k_s = {k * d_s.x, k * d_s.y, k *
    //             d_s.z};

    //             double phi = atan2(k_s.z, k_s.y) * 180.0 / M_PI;
    //             double theta = acos(k_s.x / k) * 180.0 / M_PI;

    //             size_t nplanes = par[fmat_to_par_map[p]]->nplanes;
    //             // sort phi
    //             qsort(fmat[p], nplanes, sizeof(Fmat), cmp_phi);
    //             size_t idx_low = 0;
    //             size_t idx_high = 0;
    //             for (size_t m = 0; m < nplanes - 1; ++m) {
    //                 double phi_low = fmat[p][m].phi;
    //                 double phi_high = fmat[p][m + 1].phi;
    //                 if (phi >= phi_low && phi < phi_high) {
    //                     idx_low = m;
    //                     idx_high = m + 1;
    //                     break;
    //                 }
    //             }
    //             if (idx_low == idx_high) {
    //                 // TODO: HANDLE ERROR
    //                 return;
    //             }

    //             // Interpolate between fmat[p][idx_low] and fmat[p][idx_high]
    //             for each theta
    //         }
        }
    }

    detscat_prt_free(&prt);
    detscat_ddscat_free(&ddscat);
    return;
    
cleanup_ddscat:
    detscat_prt_free(&prt);
    detscat_ddscat_free(&ddscat);
    return;
}

// double detscat_calculate_incident_field_strength(double d, double E_p_mj,
// double tau_p_ns) {
//     double A = d * d * DETSCAT_CONST_MM2M * DETSCAT_CONST_MM2M * M_PI / 4.0;
//     double I = E_p_mj * DETSCAT_CONST_MJ2J / tau_p_ns / DETSCAT_CONST_NS2S /
//     A; return sqrt(2 * I / DETSCAT_CONST_C_MS / DETSCAT_CONST_EPS0_F_M);
// }
