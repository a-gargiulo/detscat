#include "detscat_ddscat.h"

#include "detscat.h"
#include "detscat_error.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// --- Internal helpers (SHARED)
bool detscat_ddscat_init(DetScatDdscat *ddscat, size_t n_pars, size_t n_fmls,
                         size_t n_par_idxs, DetScatError *err) {
    assert(ddscat);

    if (!n_pars || !n_fmls || !n_par_idxs) {
        DETSCAT_SET_ERROR(
            err, DETSCAT_ERR_INVALID_ARG,
            "Invalid or zero size parameters provided");
        return false;
    }

    ddscat->pars = calloc(n_pars, sizeof(*ddscat->pars));
    ddscat->fmls = calloc(n_fmls, sizeof(*ddscat->fmls));
    ddscat->par_idxs = calloc(n_par_idxs, sizeof(*ddscat->par_idxs));

    if (!ddscat->pars || !ddscat->fmls || !ddscat->par_idxs) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Could not allocate memory for ddscat fields");
        free(ddscat->pars);
        free(ddscat->fmls);
        free(ddscat->par_idxs);

        ddscat->pars = NULL;
        ddscat->fmls = NULL;
        ddscat->par_idxs = NULL;

        return false;
    }

    ddscat->n_pars = n_pars;
    ddscat->n_fmls = n_fmls;
    ddscat->n_par_idxs = n_par_idxs;

    return true;
}

void detscat_ddscat_par_free(DetScatDdscatParams *par) {
    if (!par) return;

    if (par->components) {
        for (size_t i = 0; i < par->n_components; ++i) {
            detscat_str_free(&par->components[i]);
        }
        free(par->components);
        par->components = NULL;
    }

    if (par->scat_planes) {
        free(par->scat_planes);
        par->scat_planes = NULL;
    }

    par->n_components = 0;
    par->n_scat_planes = 0;
    memset(&par->e01, 0, sizeof(par->e01));
}

void detscat_ddscat_fml_free(DetScatDdscatFml *fml) {
    if (!fml) return;

    if (fml->refcount == 0) return;
    fml->refcount--;
    if (fml->refcount > 0) return;

    if (fml->fmats) {
        for (size_t i = 0; i < fml->n_fmats; ++i) {
            free(fml->fmats[i].f11);
            free(fml->fmats[i].f21);
            free(fml->fmats[i].f12);
            free(fml->fmats[i].f22);
            free(fml->fmats[i].theta);

            fml->fmats[i].f11 = NULL;
            fml->fmats[i].f21 = NULL;
            fml->fmats[i].f12 = NULL;
            fml->fmats[i].f22 = NULL;
            fml->fmats[i].theta = NULL;

            fml->fmats[i].phi = 0;
            fml->fmats[i].n_theta = 0;
        }
        free(fml->fmats);
        fml->fmats = NULL;
    }

    fml->n_fmats = 0;
}

void detscat_ddscat_destroy(DetScatDdscat *ddscat) {
    if (!ddscat) return;

    if (ddscat->pars) {
        for (size_t i = 0; i < ddscat->n_pars; ++i) {
            detscat_ddscat_par_free(&ddscat->pars[i]);
        }
        free(ddscat->pars);
        ddscat->pars = NULL;
    }

    if (ddscat->fmls) {
        for (size_t i = 0; i < ddscat->n_fmls; ++i) {
            detscat_ddscat_fml_free(&ddscat->fmls[i]);
        }
        free(ddscat->fmls);
        ddscat->fmls = NULL;
    }

    if (ddscat->par_idxs) {
        free(ddscat->par_idxs);
        ddscat->par_idxs = NULL;
    }

    ddscat->n_pars = 0;
    ddscat->n_fmls = 0;
    ddscat->n_par_idxs = 0;
}

void detscat_ddscat_par_free_subset(DetScatDdscatParams *par, size_t count) {
    if (!par) return;

    if (par->components) {
        for (size_t i = 0; i < count; ++i) {
            detscat_str_free(&par->components[i]);
        }
        free(par->components);
        par->components = NULL;
    }

    if (par->scat_planes) {
        free(par->scat_planes);
        par->scat_planes = NULL;
    }

    par->n_components = 0;
    par->n_scat_planes = 0;
    memset(&par->e01, 0, sizeof(par->e01));
}

void detscat_ddscat_fml_free_subset(DetScatDdscatFml *fml, size_t count) {
    if (!fml) return;

    if (fml->fmats) {
        for (size_t i = 0; i < count; ++i) {
            free(fml->fmats[i].f11);
            free(fml->fmats[i].f21);
            free(fml->fmats[i].f12);
            free(fml->fmats[i].f22);
            free(fml->fmats[i].theta);

            fml->fmats[i].f11 = NULL;
            fml->fmats[i].f21 = NULL;
            fml->fmats[i].f12 = NULL;
            fml->fmats[i].f22 = NULL;
            fml->fmats[i].theta = NULL;

            fml->fmats[i].phi = 0;
            fml->fmats[i].n_theta = 0;
        }
        free(fml->fmats);
        fml->fmats = NULL;
    }

    fml->n_fmats = 0;
}
