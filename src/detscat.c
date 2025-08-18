#include "detscat.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <omp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "detscat_camera.h"
#include "detscat_config.h"
#include "detscat_const.h"
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

static void detscat_vprint(const char *frmt, va_list args) {
    while (*frmt) {
        if (*frmt == '%' && *(frmt + 1)) {
            frmt++;
            switch (*frmt) {
                case 'd':
                    printf("%d", va_arg(args, int));
                    break;
                case 'c':
                    putchar((char)va_arg(args, int));
                    break;
                case 'f':
                    printf("%f", va_arg(args, double));
                    break;
                case 's':
                    printf("%s", va_arg(args, const char *));
                    break;
                case 'l':
                    if (*(frmt + 1) == 'f') {
                        frmt += 2;
                        printf("%lf", va_arg(args, double));
                        continue;
                    } else if (*(frmt + 1) == 'd') {
                        frmt += 2;
                        printf("%ld", va_arg(args, long int));
                        continue;
                    }
                    putchar('%');
                    putchar('l');
                    break;
                case 'z':
                    if (*(frmt + 1) == 'u') {
                        frmt += 2;
                        printf("%zu", va_arg(args, size_t));
                        continue;
                    }
                    putchar('%');
                    putchar('z');
                    break;
                default:
                    putchar('%');
                    putchar(*frmt);
            }
        } else {
            putchar(*frmt);
        }
        frmt++;
    }
}

void detscat_error(const char *fnct, int line, const char *file,
                   const char *frmt, ...) {
    printf("[\033[91mERROR\033[0m]: \033[95m\"%s\", line %d, in %s()\033[0m: ",
           file, line, fnct);
    va_list args;
    va_start(args, frmt);
    detscat_vprint(frmt, args);
    putchar('\n');
    va_end(args);
    return;
}

void detscat_info(const char *frmt, ...) {
    printf("[\033[94mINFO\033[0m]: ");
    va_list args;
    va_start(args, frmt);
    detscat_vprint(frmt, args);
    putchar('\n');
    va_end(args);
    return;
}

static void detscat_parse_cfg_file(const char *file_path,
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

static void detscat_parse_particles_file(DetScatParticlesData *pr_data,
                                         DetScatConfig *config,
                                         DetScatDiagnose *diagnose) {
    assert(pr_data != NULL);
    assert(config != NULL);
    assert(diagnose != NULL);

    errno = 0;
    DetScatParticlesParser *pr_parser =
        detscat_particles_parser_create(config->particles_definition_file);
    if (!pr_parser) {
        if (errno != 0)
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                 "Could not initialize particles definition "
                                 "file parser from '%s': %s",
                                 config->particles_definition_file,
                                 strerror(errno));
        else
            DETSCAT_SET_DIAGNOSE(
                *diagnose, DETSCAT_ERR_FILE_PARSING,
                "Could not initialize particles definition file parser from "
                "'%s': Failed to allocate parser.",
                config->particles_definition_file);

        return;
    }

    if (!detscat_particles_parser_parse(pr_parser, pr_data)) {
        DETSCAT_SET_DIAGNOSE(
            *diagnose, DETSCAT_ERR_FILE_PARSING, "While parsing '%s': %s",
            config->particles_definition_file, pr_parser->err_msg);
        detscat_particles_parser_free(pr_parser);
        detscat_particles_data_free(pr_data);
        return;
    }
    detscat_particles_parser_free(pr_parser);

    detscat_info("Successfully parsed '%s'.", config->particles_definition_file);
    return;
}

static int cmp_phi(const void *a, const void *b) {
    const DetScatDdscatFmatrix *fa = (const DetScatDdscatFmatrix *)a;
    const DetScatDdscatFmatrix *fb = (const DetScatDdscatFmatrix *)b;
    return (fa->phi > fb->phi) - (fa->phi < fb->phi);
}

static void detscat_fetch_ddscat_data(DetScatDdscatData *ddscat,
                                      DetScatParticlesData *pr_data,
                                      DetScatDiagnose *diagnose) {
    assert(ddscat != NULL);
    assert(diagnose != NULL);
    assert(pr_data != NULL);

    assert(pr_data->n_particles > 0);
    assert(pr_data->n_types > 0);

    DetScatDdscatParser *ddscat_parser = NULL;

    ddscat->n_pars = pr_data->n_types;
    ddscat->pars = calloc(ddscat->n_pars, sizeof(DetScatDdscatParams));

    ddscat->n_fmls = pr_data->n_particles;
    ddscat->fmls = calloc(ddscat->n_fmls, sizeof(DetScatDdscatFml));

    ddscat->n_par_idx = pr_data->n_particles; 
    ddscat->par_idx = calloc(ddscat->n_par_idx, sizeof(size_t));

    if (!ddscat->pars || !ddscat->fmls || !ddscat->par_idx) {
        detscat_ddscat_data_free(ddscat);
        detscat_particles_data_free(pr_data);
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_ALLOC, "%s",
                             "Could not allocate DDSCAT data container.");
        return;
    }

    // PAR LOOP
    bool par_parser_created = false;
    for (size_t i = 0; i < pr_data->n_types; ++i) {
        char par_file_path[DETSCAT_PATH_MAX];

        strcpy(par_file_path, pr_data->types[i].data_dir);

        if (par_file_path[strlen(par_file_path) - 1] == '/')
            strcat(par_file_path, "ddscat.par");
        else
            strcat(par_file_path, "/ddscat.par");

        if (!par_parser_created) {
            errno = 0;
            ddscat_parser = detscat_ddscat_parser_create(par_file_path);
            if (!ddscat_parser) {
                if (errno != 0)
                    DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                         "Could not initialize the par file "
                                         "parser from '%s': %s",
                                         par_file_path, strerror(errno));
                else
                    DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                         "Could not initialize par file "
                                         "parser from '%s': Failed to allocate parser.",
                                         par_file_path);
                
                detscat_ddscat_data_free(ddscat);
                detscat_particles_data_free(pr_data);
                return;
            }
        } else {
            assert(ddscat_parser != NULL);
            if (!detscat_ddscat_parser_reset(ddscat_parser, par_file_path)) {
                DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                     "Could not reset par file "
                                     "parser: %s",
                                     ddscat_parser->err_msg);
                detscat_ddscat_data_free(ddscat);
                detscat_particles_data_free(pr_data);
                detscat_ddscat_parser_free(ddscat_parser);
                return;
            }
        }

        if (!detscat_ddscat_parser_parse_par(ddscat_parser, &ddscat->pars[i])) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                 "Could not parse the DDSCAT parameter file "
                                 "'%s' for particle type "
                                 "'%s'.",
                                 par_file_path,
                                 pr_data->types[i].type_id);
            detscat_ddscat_data_free(ddscat);
            detscat_particles_data_free(pr_data);
            detscat_ddscat_parser_free(ddscat_parser);
            return;
        }
    }

    // FML LOOP
    for (size_t i = 0; i < pr_data->n_particles; ++i) {
        // note: fml files are named w000r000k000.fml, where
        // w = wavelength,
        // r = radius,
        // k = orientation.

        // TODO: Could implement a hash table for faster lookup
        // -----
        size_t idx = (size_t)(-1);
        for (size_t j = 0; j < pr_data->n_types; ++j) {
            if (strcmp(pr_data->particles[i].type_id, pr_data->types[j].type_id) == 0) {
                idx = j;
                break;
            }
        }

        if (idx == (size_t)(-1)) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_LOOKUP,
                                 "Particle type '%s' not found in definitions.",
                                 pr_data->particles[i].type_id);
            detscat_ddscat_data_free(ddscat);
            detscat_particles_data_free(pr_data);
            detscat_ddscat_parser_free(ddscat_parser);
            return;
        }
        ddscat->par_idx[i] = idx;
        // -----

        char fml_file_path[DETSCAT_PATH_MAX];
        char fml_name[64];
        strcpy(fml_file_path, pr_data->types[idx].data_dir);
        if (fml_file_path[strlen(fml_file_path) - 1] == '/') {
            sprintf(fml_name, "w%03dr%03dk%03d.fml",
                    pr_data->particles[i].case_id.w, 
                    pr_data->particles[i].case_id.r,
                    pr_data->particles[i].case_id.k);
            strcat(fml_file_path, fml_name);
        } else {
            sprintf(fml_name, "/w%03dr%03dk%03d.fml",
                    pr_data->particles[i].case_id.w,
                    pr_data->particles[i].case_id.r,
                    pr_data->particles[i].case_id.k);
            strcat(fml_file_path, fml_name);
        }

        assert(ddscat_parser != NULL);
        if (!detscat_ddscat_parser_reset(ddscat_parser, fml_file_path)) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                 "Could not reset fml file "
                                 "parser: %s",
                                 ddscat_parser->err_msg);
            detscat_ddscat_data_free(ddscat);
            detscat_particles_data_free(pr_data);
            detscat_ddscat_parser_free(ddscat_parser);
            return;
        }

        if (!detscat_ddscat_parser_parse_fml(ddscat_parser, &ddscat->fmls[i], &ddscat->pars[idx])) {
            DETSCAT_SET_DIAGNOSE(
                *diagnose, DETSCAT_ERR_FILE_PARSING,
                "Could not parse the DDSCAT fml file '%s' for particle number "
                "'%zu'.",
                fml_file_path, i + 1);
            detscat_ddscat_data_free(ddscat);
            detscat_particles_data_free(pr_data);
            detscat_ddscat_parser_free(ddscat_parser);
            return;
        }
    }

    detscat_ddscat_parser_free(ddscat_parser);
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

void detscat_run(int argc, char **argv, DetScatDiagnose *diagnose) {
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
    DetScatParticlesData pr = {0};
    DetScatDdscatData ddscat = {0};
    DetScatCamera camera = {0};
    DetScatImage image = {0};



    const char *cfg_file_path = argv[1];
    detscat_parse_cfg_file(cfg_file_path, &cfg, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

    detscat_parse_particles_file(&pr, &config, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

    detscat_fetch_ddscat_data(&ddscat, &pr, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

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

    detscat_ddscat_data_free(&ddscat);
    detscat_particles_data_free(&pr);
    return;
}

// double detscat_calculate_incident_field_strength(double d, double E_p_mj,
// double tau_p_ns) {
//     double A = d * d * DETSCAT_CONST_MM2M * DETSCAT_CONST_MM2M * M_PI / 4.0;
//     double I = E_p_mj * DETSCAT_CONST_MJ2J / tau_p_ns / DETSCAT_CONST_NS2S /
//     A; return sqrt(2 * I / DETSCAT_CONST_C_MS / DETSCAT_CONST_EPS0_F_M);
// }
