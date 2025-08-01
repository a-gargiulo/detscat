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
#include "detscat_ddscat_util.h"
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

void detscat_error(const char *fnct, int line, const char *file, const char *frmt, ...) {
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

static void detscat_parse_config_file(const char *config_file_path, DetScatConfig *config, DetScatDiagnose *diagnose) {

    assert(config_file_path != NULL && config_file_path[0] != '\0');
    assert(config != NULL);
    assert(diagnose != NULL);

    errno = 0;
    DetScatConfigParser *config_parser = detscat_config_parser_create(config_file_path);
    if (!config_parser) {
        if (errno != 0)
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                 "Could not initialize the configuration file parser from '%s': %s",
                                 config_file_path,
                                 strerror(errno));
        else
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                 "Could not initialize configuration file parser from '%s': Failed to allocate parser.",
                                 config_file_path);
        return;
    }

    if (!detscat_config_parser_parse(config_parser, config)) {
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                             "While parsing '%s': %s",
                             config_file_path, config_parser->err_msg);
        detscat_config_parser_free(config_parser);
        return;
    }
    detscat_config_parser_free(config_parser);

    detscat_info("Successfully parsed '%s'.", config_file_path);
    return;
}

static void detscat_parse_particles_file(DetScatParticlesData *particles_data, DetScatConfig *cfg, DetScatDiagnose *diagnose) {

    assert(particles_data != NULL);
    assert(cfg != NULL);
    assert(diagnose != NULL);

    errno = 0;
    DetScatParticlesParser *particles_parser = detscat_particles_parser_create(cfg->particles_definition_file);
    if (!particles_parser) {
        if (errno != 0)
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                 "Could not initialize particles definition file parser from '%s': %s",
                                 cfg->particles_definition_file,
                                 strerror(errno));
        else
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                 "Could not initialize particles definition file parser from '%s': Failed to allocate parser.",
                                 cfg->particles_definition_file);

        return;
    }

    if (!detscat_particles_parser_parse(particles_parser, particles_data)) {
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING, "While parsing '%s': %s",
                             cfg->particles_definition_file, particles_parser->err_msg);
        detscat_particles_parser_free(particles_parser);
        detscat_particles_data_free(particles_data);
        return;
    }
    detscat_particles_parser_free(particles_parser);

    detscat_info("Successfully parsed '%s'.", cfg->particles_definition_file);
    return;
}

static int cmp_phi(const void *a, const void *b) {
    const Fmat *fa = (const Fmat *)a;
    const Fmat *fb = (const Fmat *)b;
    return (fa->phi > fb->phi) - (fa->phi < fb->phi);
}

static void detscat_fetch_ddscat_data(DdscatPar ***par, Fmat ***fmat, size_t **map,
                                      DetScatParticlesData *particles_data,
                                      DetScatDiagnose *diagnose) {
    assert(particles_data != NULL);
    assert(diagnose != NULL);

    assert(particles_data->n_particles > 0);
    assert(particles_data->n_definitions > 0);

    *fmat = malloc(particles_data->n_particles * sizeof(Fmat *));
    *map = malloc(particles_data->n_particles * sizeof(size_t));
    *par = malloc(particles_data->n_definitions * sizeof(DdscatPar *));

    if (!*par || !*fmat || !*map) {
        if (*par) {
            free(*par);
            *par = NULL;
        }
        if (*fmat) {
            free(*fmat);
            *fmat = NULL;
        }
        if (*map) {
            free(*map);
            *map = NULL;
        }
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_ALLOC, "%s",
                             "Could not allocate DDSCAT data containers.");
        return;
    }

    DetScatDdscatUtilStatus status;

    for (size_t i = 0; i < particles_data->n_definitions; ++i) {
        char par_file_path[DETSCAT_PATH_MAX];
        strcpy(par_file_path, particles_data->definitions[i].data_dir);

        if (par_file_path[strlen(par_file_path) - 1] == '/')
            strcat(par_file_path, "ddscat.par");
        else
            strcat(par_file_path, "/ddscat.par");

        status = detscat_ddscat_util_parse_par_file(par_file_path, (*par)[i]);
        if (status != DETSCAT_DDSCAT_UTIL_OK) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                 "Could not parse the DDSCAT parameter file '%s' for particle type "
                                 "'%s'. Parser failed with error code: %d.",
                                 par_file_path, particles_data->definitions[i].id, status);
            free(*par);
            free(*fmat);
            free(*map);
            *par = NULL;
            *fmat = NULL;
            *map = NULL;
            return;
        }
    }

    for (size_t i = 0; i < particles_data->n_particles; ++i) {
        // // note: fml naming: w000r000k000.fml, where
        // // w = wavelength
        // // r = radius
        // // k = orientation

        // TODO: Could implement a hash table for faster lookup
        // -----
        size_t idx = (size_t)(-1);
        for (size_t j = 0; j < particles_data->n_definitions; ++j) {
            if (strcmp(particles_data->particles[i].id, particles_data->definitions[j].id) == 0) {
                idx = j;
                break;
            }
        }

        if (idx == (size_t)(-1)) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_LOOKUP,
                                 "Particle type '%s' not found in definitions.",
                                 particles_data->particles[i].id);
            free(*fmat);
            free(*par);
            free(*map);
            *fmat = NULL;
            *par = NULL;
            *map = NULL;
            return;
        }
        *map[i] = idx;
        // -----

        char fml_file_path[DETSCAT_PATH_MAX];
        char fml_name[64];
        strcpy(fml_file_path, particles_data->definitions[idx].data_dir);
        if (fml_file_path[strlen(fml_file_path) - 1] == '/') {
            sprintf(fml_name, "w%03dr%03dk%03d.fml", particles_data->particles[i].w,
                    particles_data->particles[i].r, particles_data->particles[i].k);
            strcat(fml_file_path, fml_name);
        } else {
            sprintf(fml_name, "/w%03dr%03dk%03d.fml", particles_data->particles[i].w,
                    particles_data->particles[i].r, particles_data->particles[i].k);
            strcat(fml_file_path, fml_name);
        }

        // fmat is an array of size n_particles. Here, this is *fmat.
        // each element of fmat is an array of size n_phi (azimuthal angle) of Fmat (f-matrix)
        // each Fmat contains arrays (f11, f12, f21, f22) of size n_theta (scattering angle)
        // The following function
        // DetScatDdscatUtilStatus detscat_ddscat_util_parse_fml_file(const char *fml_file_path,
        // const DdscatPar *par, Fmat **fmat); allocates the array of size n_phi
        status = detscat_ddscat_util_parse_fml_file(fml_file_path, (*par)[idx], (*fmat + i));
        if (status != DETSCAT_DDSCAT_UTIL_OK) {
            DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING,
                                 "Could not parse the DDSCAT fml file '%s' for particle number "
                                 "'%zu'. Parser failed with error code: %d.",
                                 fml_file_path, i, status);
            free(*par);
            free(*fmat);
            free(*map);
            *par = NULL;
            *fmat = NULL;
            *map = NULL;
            return;
        }
    }

    detscat_info("Successfully fetched all DDSCAT data.");
    return;
}

static void detscat_ddscat_data_free(DdscatPar **par, Fmat **fmat, size_t *fmat_to_par_map,
                                     size_t n_fmat, size_t n_par) {
    for (size_t i = 0; i < n_fmat; ++i) {
        for (size_t j = 0; j < par[fmat_to_par_map[i]]->nplanes; ++j) {
            free(fmat[i][j].f11);
            free(fmat[i][j].f21);
            free(fmat[i][j].f12);
            free(fmat[i][j].f22);
            free(fmat[i][j].theta);
        }
        free(fmat[i]);
    }
    free(fmat);
    fmat = NULL;

    for (size_t i = 0; i < n_par; ++i) {
        for (size_t j = 0; j < par[i]->ncomp; ++j) {
            free(par[i]->comp[j]);
        }
        free(par[i]->comp);
        free(par[i]->planes);
        free(par[i]);
    }
    free(par);
    free(fmat_to_par_map);
    par = NULL;
    fmat_to_par_map = NULL;
}

void detscat_run(int argc, char **argv, DetScatDiagnose *diagnose) {
    assert(diagnose != NULL);

    detscat_print_banner();

    if (argc < 2 || argv[1][0] == '\0') {
        DETSCAT_SET_DIAGNOSE(
            *diagnose, DETSCAT_ERR_MISSING_CMD_ARG,
            "Missing command-line argument. Specify configuration file. Usage: %s <path_to_configuration_file>",
            argv[0]);
        return;
    }
    detscat_info("DetScat initialized successfully.");

    DetScatConfig config = {0};
    DetScatParticlesData particles_data = {0};

    DdscatPar **par = NULL; // one parameter file per definition.
    Fmat **fmat = NULL;
    size_t *fmat_to_par_map = NULL;

    // size_t n_par = particles_data.n_definitions;
    // size_t n_fmat = particles_data.n_particles;
    // size_t n_fmat_to_par_map = n_fmat;

    // Camera *camera = detscat_camera_create(&config);
    // Image *image = detscat_camera_image_create(camera->width, camera->height);

    const char *config_file_path = argv[1];
    detscat_parse_config_file(config_file_path, &config, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

    detscat_parse_particles_file(&particles_data, &config, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

    detscat_fetch_ddscat_data(&par, &fmat, &fmat_to_par_map, &particles_data, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

    // // MAIN LOOP
    // #pragma omp parallel for collapse(2)
    // for (int u = 0; u < image->width; ++u) {
    //     for (int v = 0; v < image->height; ++v) {
    //         int pxl_idx = detscat_camera_get_image_index(image, u, v);
    //         Vec3 x_pxl_w;
    //         detscat_camera_pixel_coordinate_to_world(camera, &x_pxl_w, u, v);

    //         for (size_t p = 0; p < particles_data.n_particles; ++p) {
    //             Vec3 x_s_w;
    //             mymath_vec3_sub(&x_s_w, &x_pxl_w, &particles_data.particles[p].position);
    //             double x_s_w_abs = mymath_vec3_abs(&x_s_w);

    //             Vec3 d_s;
    //             mymath_vec3_normalize(&d_s, &x_s_w, x_s_w_abs);
    //             double k = 2.0 * M_PI / (config.wavelength_nm * DETSCAT_CONST_NM2M);
    //             Vec3 k_s = {k * d_s.x, k * d_s.y, k * d_s.z};

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

    //             // Interpolate between fmat[p][idx_low] and fmat[p][idx_high] for each theta
    //         }
    //     }
    // }

    // detscat_ddscat_data_free(par, fmat, fmat_to_par_map, n_fmat, n_par);
    // detscat_particles_free(&particles_data);
    return;
}

// double detscat_calculate_incident_field_strength(double d, double E_p_mj, double tau_p_ns) {
//     double A = d * d * DETSCAT_CONST_MM2M * DETSCAT_CONST_MM2M * M_PI / 4.0;
//     double I = E_p_mj * DETSCAT_CONST_MJ2J / tau_p_ns / DETSCAT_CONST_NS2S / A;
//     return sqrt(2 * I / DETSCAT_CONST_C_MS / DETSCAT_CONST_EPS0_F_M);
// }
