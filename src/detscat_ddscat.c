#include "detscat_ddscat.h"

#include "detscat.h"
#include "detscat_error.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// --- Internal helpers (PRIVATE) ---
static Complex detscat_ddscat_linterp_theta_array(double theta,
                                                 double *theta_arr,
                                                 Complex *values,
                                                 size_t n) {
    assert(theta_arr && values);

    // Bracket theta
    size_t idx_low, idx_high;
    detscat_math_bracket_value(theta_arr, n, sizeof(double), 0, theta,
                               &idx_low, &idx_high);

    return detscat_math_cplx_linterp(theta,
                                     values[idx_low], values[idx_high],
                                     theta_arr[idx_low], theta_arr[idx_high]);
}



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

    if (par->orientations) {
        free(par->orientations);
        par->orientations = NULL;
    }

    if (par->wavelengths) {
        free(par->wavelengths);
        par->wavelengths = NULL;
    }

    if (par->radii) {
        free(par->radii);
        par->radii= NULL;
    }

    if (par->cases) {
        free(par->cases);
        par->cases = NULL;
    }

    par->n_components = 0;
    par->n_scat_planes = 0;
    par->n_cases = 0;
    par->n_orientations = 0;
    par->n_wavelengths = 0;
    par->n_radii = 0;
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

    free(fml);
}

void detscat_ddscat_free(DetScatDdscat *ddscat) {
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
            if (ddscat->fmls[i]) {
                detscat_ddscat_fml_free(ddscat->fmls[i]);
            }
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

    if (par->orientations) {
        free(par->orientations);
        par->orientations= NULL;
    }

    if (par->wavelengths) {
        free(par->wavelengths);
        par->wavelengths = NULL;
    }

    if (par->radii) {
        free(par->radii);
        par->radii= NULL;
    }

    if (par->cases) {
        free(par->cases);
        par->cases = NULL;
    }

    par->n_components = 0;
    par->n_scat_planes = 0;
    par->n_cases = 0;
    par->n_orientations = 0;
    par->n_wavelengths = 0;
    par->n_radii = 0;
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


ComplexMat2 detscat_ddscat_get_fmatrix(const DetScatDdscatFml *fml, double phi, double theta) {
    ComplexMat2 F = (ComplexMat2){0};
    
    // Bracket phi 
    size_t phi_low, phi_high;
    detscat_math_bracket_value(
        fml->fmats,
        fml->n_fmats,
        sizeof(DetScatDdscatFmatrix),
        offsetof(DetScatDdscatFmatrix, phi),
        phi,
        &phi_low,
        &phi_high);

    const DetScatDdscatFmatrix *fm_low  = &fml->fmats[phi_low];
    const DetScatDdscatFmatrix *fm_high = &fml->fmats[phi_high];

    Complex f11_low = detscat_ddscat_linterp_theta_array(
        theta, fm_low->theta, fm_low->f11, fm_low->n_theta);
    Complex f12_low = detscat_ddscat_linterp_theta_array(
        theta, fm_low->theta, fm_low->f12, fm_low->n_theta);
    Complex f21_low = detscat_ddscat_linterp_theta_array(
        theta, fm_low->theta, fm_low->f21, fm_low->n_theta);
    Complex f22_low = detscat_ddscat_linterp_theta_array(
        theta, fm_low->theta, fm_low->f22, fm_low->n_theta);

    Complex f11_high = detscat_ddscat_linterp_theta_array(
        theta, fm_high->theta, fm_high->f11, fm_high->n_theta);
    Complex f12_high = detscat_ddscat_linterp_theta_array(
        theta, fm_high->theta, fm_high->f12, fm_high->n_theta);
    Complex f21_high = detscat_ddscat_linterp_theta_array(
        theta, fm_high->theta, fm_high->f21, fm_high->n_theta);
    Complex f22_high = detscat_ddscat_linterp_theta_array(
        theta, fm_high->theta, fm_high->f22, fm_high->n_theta);

    F.f11 = detscat_math_cplx_linterp(
        phi, f11_low, f11_high, fm_low->phi, fm_high->phi);
    F.f12 = detscat_math_cplx_linterp(
        phi, f12_low, f12_high, fm_low->phi, fm_high->phi);
    F.f21 = detscat_math_cplx_linterp(
        phi, f21_low, f21_high, fm_low->phi, fm_high->phi);
    F.f22 = detscat_math_cplx_linterp(
        phi, f22_low, f22_high, fm_low->phi, fm_high->phi);

    return F;
}
