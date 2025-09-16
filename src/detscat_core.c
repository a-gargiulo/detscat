#include "detscat.h"

#include "detscat_camera.h"
#include "detscat_cfg.h"
#include "detscat_ddscat.h"
#include "detscat_error.h"
#include "detscat_log.h"
#include "detscat_math.h"
#include "detscat_prt.h"
#include "detscat_str.h"
#include "detscat_transform.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct DetScat {
    const char *cfg_file_path;
    DetScatConfig cfg;
    DetScatPrt prt;
    DetScatDdscat ddscat;
    DetScatCamera cam;
    DetScatTransform transform;
};

typedef struct {
    Str type_id;
    DetScatPrtCaseId case_id;
    DetScatDdscatFml *fml;
} DetScatFmlCacheEntry;

typedef struct {
    DetScatFmlCacheEntry *entries;
    size_t n_entries;
} DetScatFmlCache;

// --- Static helper (PRIVATE) ---
static void detscat_banner_print(FILE *out) {
    if (!out) out = stdout;

    fprintf(out,
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

static void vec3_print(const Vec3 *v) {
    printf("[%10.6f, %10.6f, %10.6f ]", v->x, v->y, v->z);
}

static void complexvec3_print(const ComplexVec3 *v) {
    printf("[%.6f+%.6fi, %.6f+%.6fi, %.6f+%.6fi]", v->x.re, v->x.im, v->y.re,
           v->y.im, v->z.re, v->z.im);
}

static void str_print(const Str *s) {
    printf("\"%s\"", s->data);
}

static bool find_type_index(const DetScatPrt *prt, const char *type_id,
                            size_t *out_idx) {
    for (size_t j = 0; j < prt->n_types; ++j) {
        if (strcmp(type_id, prt->types[j].type_id.data) == 0) {
            *out_idx = j;
            return true;
        }
    }
    return false;
}

static DetScatDdscatFml *detscat_get_cached_fml(DetScatFmlCache *cache,
                                                Str *type_id,
                                                DetScatPrtCaseId *case_id) {
    for (size_t i = 0; i < cache->n_entries; ++i) {
        if (strcmp(cache->entries[i].type_id.data, type_id->data) == 0 &&
            cache->entries[i].case_id.w == case_id->w &&
            cache->entries[i].case_id.r == case_id->r &&
            cache->entries[i].case_id.k == case_id->k) {
            return cache->entries[i].fml;  // already parsed
        }
    }
    return NULL;  // not in cache
}

static void detscat_cache_fml(DetScatFmlCache *cache, Str *type_id,
                              DetScatPrtCaseId *case_id,
                              DetScatDdscatFml *fml) {

    cache->entries = realloc(
        cache->entries, sizeof(DetScatFmlCacheEntry) * (cache->n_entries + 1));
    cache->entries[cache->n_entries].type_id = (Str){0};
    detscat_str_copy(&cache->entries[cache->n_entries].type_id, type_id);
    cache->entries[cache->n_entries].case_id.w = case_id->w;
    cache->entries[cache->n_entries].case_id.r = case_id->r;
    cache->entries[cache->n_entries].case_id.k = case_id->k;
    cache->entries[cache->n_entries].fml = fml;
    cache->n_entries++;
}

static void detscat_cache_free_fml(DetScatFmlCache *cache) {
    if (!cache) return;

    for (size_t i = 0; i < cache->n_entries; ++i) {
        detscat_str_free(&cache->entries[i].type_id);
    }
    free(cache->entries);  // free only the array of entries
    cache->entries = NULL;
    cache->n_entries = 0;
}

static void detscat_fml_acquire(DetScatDdscatFml *fml) {
    if (!fml) return;

    assert(fml->refcount > 0);

    fml->refcount++;
}

// --- Public API ---
bool detscat_init(DetScatError *err) {
    detscat_log_init_lock();
    detscat_banner_print(NULL);
    return true;
}

void detscat_shutdown(void) {
    detscat_log_destroy_lock();
}

DetScat *detscat_create(const char *cfg_file_path, DetScatError *err) {
    if (!cfg_file_path || !*cfg_file_path) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_INVALID_ARG,
                          "Invalid cfg file path");
        return NULL;
    }

    DetScat *detscat = calloc(1, sizeof(*detscat));
    if (!detscat) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Could not allocate memory for DetScat context");
        return NULL;
    }

    detscat->cfg_file_path = cfg_file_path;
    detscat->cfg = (DetScatConfig){0};
    detscat->prt = (DetScatPrt){0};
    detscat->ddscat = (DetScatDdscat){0};
    detscat->cam = (DetScatCamera){0};
    detscat->transform = (DetScatTransform){0};

    if (!detscat_cfg_init(&detscat->cfg, err)) goto error_cleanup;

    if (!detscat_cfg_load(cfg_file_path, &detscat->cfg, err))
        goto error_cleanup;

    return detscat;

error_cleanup:
    detscat_cfg_free(&detscat->cfg);
    free(detscat);
    return NULL;
}

void detscat_destroy(DetScat **detscat) {
    if (!detscat || !*detscat) return;

    DetScat *d = *detscat;

    detscat_cfg_free(&d->cfg);
    detscat_prt_free(&d->prt);
    detscat_ddscat_free(&d->ddscat);
    detscat_camera_free(&d->cam);

    *d = (DetScat){0};

    free(d);
    *detscat = NULL;
}

bool detscat_load_data(DetScat *detscat, DetScatError *err) {
    assert(detscat);

    if (!detscat) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_INVALID_ARG,
                          "DetScat object does not exist / is null");
        return false;
    }

    if (!detscat->cfg.particles_file_path.data ||
        !*detscat->cfg.particles_file_path.data) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_INVALID_ARG,
                          "Invalid particles file path");
        return false;
    }
    const char *prt_file_path = detscat->cfg.particles_file_path.data;

    // PRT
    if (!detscat_prt_load(prt_file_path, &detscat->prt, err)) goto cleanup;

    detscat_math_vec3_centroid(&detscat->transform.translation,
                               detscat->prt.particles,
                               detscat->prt.n_particles,
                               sizeof(DetScatPrtParticle),
                               offsetof(DetScatPrtParticle, position));


    Vec3 xprime, yprime, zprime;
    detscat_math_vec3_build_basis(&detscat->cfg.light_source_direction,
                                  &(Vec3){0, 1, 0},
                                  AXIS_X,
                                  &xprime,
                                  &yprime,
                                  &zprime);
    detscat_math_mat3_basis_to_rotmat(&detscat->transform.rotation, 
                                      &xprime, &yprime, &zprime);

    detscat_prt_transform(detscat->prt.particles,
                          detscat->prt.n_particles,
                          &detscat->transform.translation,
                          &detscat->transform.rotation);

    // DDSCAT
    DetScatFmlCache fml_cache = {0};

    size_t n_pars = detscat->prt.n_types;
    size_t n_fmls = detscat->prt.n_particles;
    size_t n_par_idxs = detscat->prt.n_particles;
    if (!detscat_ddscat_init(&detscat->ddscat, n_pars, n_fmls, n_par_idxs, err)) goto cleanup_ddscat;

    // PARS
    for (size_t i = 0; i < n_pars; ++i) {
        const char *par_dir = detscat->prt.types[i].data_dir.data;

        Str par_file_path = {0};
        if (!detscat_str_init_fmt(
            &par_file_path, "%s%sddscat.par", par_dir,
            (par_dir[0] && par_dir[strlen(par_dir) - 1] != '/') ? "/" : "")) {
            DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                              "Could not allocate memory for .par file path");
            detscat_str_free(&par_file_path);
        }

        if (!detscat_ddscat_par_load(par_file_path.data,
                                     &detscat->ddscat.pars[i], err)) {
            detscat_str_free(&par_file_path);
            goto cleanup_ddscat;
        }

        detscat_str_free(&par_file_path);
    }

    // FMLS + PAR_IDXS
    for (size_t i = 0; i < detscat->prt.n_particles; ++i) {
        size_t idx;
        if (!find_type_index(&detscat->prt,
                             detscat->prt.particles[i].type_id.data, &idx)) {
            DETSCAT_SET_ERROR(err, DETSCAT_ERR_KEY_LOOKUP,
                              "Particle type '%s' not found in definitions",
                              detscat->prt.particles[i].type_id.data);
            goto cleanup_ddscat;
        }
        detscat->ddscat.par_idxs[i] = idx;

        const char *fml_dir = detscat->prt.types[idx].data_dir.data;

        Str fml_file_path = {0};
        if (!detscat_str_init_fmt(
            &fml_file_path, "%s%sw%03dr%03dk%03d.fml", fml_dir,
            (fml_dir[0] && fml_dir[strlen(fml_dir) - 1] != '/') ? "/" : "",
            detscat->prt.particles[i].case_id.w,
            detscat->prt.particles[i].case_id.r,
            detscat->prt.particles[i].case_id.k)) {
            
            DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                              "Could not allocate memory for .fml file path");
            detscat_str_free(&fml_file_path);
            goto cleanup_ddscat;
        }

        // check cache
        // for each particle, check if type_id and case_id were cached
        // retrieve corresponding fml file --> pointer to fml struct
        DetScatDdscatFml *cached_fml = detscat_get_cached_fml(
            &fml_cache, &detscat->prt.particles[i].type_id,
            &detscat->prt.particles[i].case_id);

        if (cached_fml) {
            detscat_fml_acquire(cached_fml);
            // reuse cached fml
            detscat->ddscat.fmls[i] = cached_fml; // cache was wrongly copied before
        } else {
            // allocate fml struct on demand
            DetScatDdscatFml *new_fml = calloc(1, sizeof(*new_fml));
            if (!new_fml) {
                DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                                  "Could not allocate memory for temporary fml handle");
                detscat_str_free(&fml_file_path);
                goto cleanup_ddscat;
            }

            // new_fml->refcount = 1

            if (!detscat_ddscat_fml_load(fml_file_path.data,
                                         new_fml,
                                         &detscat->ddscat.pars[idx], err)) {
                free(new_fml);
                detscat_str_free(&fml_file_path);
                goto cleanup_ddscat;
            }

            // store in cache for future reuse
            detscat_cache_fml(&fml_cache, &detscat->prt.particles[i].type_id,
                              &detscat->prt.particles[i].case_id,
                              new_fml);
            detscat->ddscat.fmls[i] = new_fml;
        }

        detscat_str_free(&fml_file_path);
    }

    detscat_cache_free_fml(&fml_cache);
    detscat_log(DETSCAT_INFO, "Successfully loaded simulation data");
    return true;

cleanup_ddscat:
    detscat_cache_free_fml(&fml_cache);
    detscat_ddscat_free(&detscat->ddscat);
cleanup:
    detscat_prt_free(&detscat->prt);
    return false;
}

bool detscat_setup_camera(DetScat *detscat, DetScatError *err) {

    if (!detscat_camera_init(&detscat->cam, &detscat->cfg, &detscat->transform, err)) return false;

    return true;

}














void detscat_print_cfg(const DetScat *detscat) {
    if (!detscat) return;

    printf("\n");
    printf("        Parsed CFG Data:\n");
    printf("        ----------------\n");

    // For strings, use         %-25s to left-align within 25 chars
    printf("        %-25s: \"%s\"\n", "particles_file_path",
           detscat->cfg.particles_file_path.data);

    // Align doubles
    printf("        %-25s: %10.6f\n", "wavelength_nm",
           detscat->cfg.wavelength_nm);
    printf("        %-25s: %10.6f\n", "pulse_energy_mj",
           detscat->cfg.pulse_energy_mj);
    printf("        %-25s: %10.6f\n", "pulse_width_ns",
           detscat->cfg.pulse_width_ns);
    printf("        %-25s: %10.6f\n", "beam_diameter_mm",
           detscat->cfg.beam_diameter_mm);

    // Vec3
    printf("        %-25s: ", "camera_center_position_m");
    vec3_print(&detscat->cfg.camera_center_position_m);
    printf("\n");
    printf("        %-25s: ", "camera_direction");
    vec3_print(&detscat->cfg.camera_direction);
    printf("\n");

    // More doubles
    printf("        %-25s: %10.6f\n", "focal_length_mm",
           detscat->cfg.focal_length_mm);
    printf("        %-25s: %10.6f\n", "sensor_width_mm",
           detscat->cfg.sensor_width_mm);
    printf("        %-25s: %10.6f\n", "sensor_height_mm",
           detscat->cfg.sensor_height_mm);

    // Integers
    printf("        %-25s: %6d\n", "camera_resolution_x_px",
           detscat->cfg.sensor_resolution_x_px);
    printf("        %-25s: %6d\n", "camera_resolution_y_px",
           detscat->cfg.sensor_resolution_y_px);

    printf("\n");
}

// TODO: expand
void detscat_print_prt(const DetScat *detscat) {
    if (!detscat) return; 

    for (size_t i = 0; i < detscat->prt.n_particles; ++i) {
        printf("%s %d %d %d %lf %lf %lf\n",
               detscat->prt.particles[i].type_id.data,
               detscat->prt.particles[i].case_id.w,
               detscat->prt.particles[i].case_id.r,
               detscat->prt.particles[i].case_id.k,
               detscat->prt.particles[i].position.x,
               detscat->prt.particles[i].position.y,
               detscat->prt.particles[i].position.z);
    }
}

// TODO: expand
void detscat_print_ddscat(const DetScat *detscat) {
    if (!detscat) return; 

    for (size_t i = 0; i < detscat->ddscat.n_pars; ++i) {
        printf("PAR %zu\n", i);
        printf("\n");
        printf("        %-25s: ", "e01");
        complexvec3_print(&detscat->ddscat.pars[i].e01);
        printf("\n");

        printf("        %-25s: %zu\n", "#Components",
               detscat->ddscat.pars[i].n_components);
        for (size_t j = 0; j < detscat->ddscat.pars[i].n_components; ++j) {
            printf("        %-25s  %s\n", "",
                   detscat->ddscat.pars[i].components[j].data);
        }

        printf("        %-25s: %zu\n", "#Scattering Planes",
               detscat->ddscat.pars[i].n_scat_planes);
        for (size_t j = 0; j < detscat->ddscat.pars[i].n_scat_planes; ++j) {
            printf("        %-25s  %lf %lf %lf %lf\n", "",
                   detscat->ddscat.pars[i].scat_planes[j][0],
                   detscat->ddscat.pars[i].scat_planes[j][1],
                   detscat->ddscat.pars[i].scat_planes[j][2],
                   detscat->ddscat.pars[i].scat_planes[j][3]);
        }
    }

    for (size_t i = 0; i < detscat->ddscat.n_fmls; ++i) {
        for (size_t j = 0; j < detscat->ddscat.fmls[i]->n_fmats; ++j) {
            for (size_t k = 0; k < detscat->ddscat.fmls[i]->fmats[j].n_theta;
                 ++k) {
                printf(
                    "%.1lf %.1lf %10.3e %10.3e %10.3e %10.3e %10.3e %10.3e "
                    "%10.3e %10.3e\n",
                    detscat->ddscat.fmls[i]->fmats[j].theta[k],
                    detscat->ddscat.fmls[i]->fmats[j].phi,
                    detscat->ddscat.fmls[i]->fmats[j].f11[k].re,
                    detscat->ddscat.fmls[i]->fmats[j].f11[k].im,
                    detscat->ddscat.fmls[i]->fmats[j].f21[k].re,
                    detscat->ddscat.fmls[i]->fmats[j].f21[k].im,
                    detscat->ddscat.fmls[i]->fmats[j].f12[k].re,
                    detscat->ddscat.fmls[i]->fmats[j].f12[k].im,
                    detscat->ddscat.fmls[i]->fmats[j].f22[k].re,
                    detscat->ddscat.fmls[i]->fmats[j].f22[k].im);
            }
        }
    }
}





