#include "detscat_ddscat.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mymath.h"
#include "strutil.h"

DetScatDdscatParser *detscat_ddscat_parser_create(
    const char *ddscat_file_path) {
    assert(ddscat_file_path != NULL && ddscat_file_path[0] != '\0');

    DetScatDdscatParser *ddscat_parser = malloc(sizeof(DetScatDdscatParser));
    if (!ddscat_parser) return NULL;

    ddscat_parser->file = fopen(ddscat_file_path, "r");
    if (!ddscat_parser->file) {
        free(ddscat_parser);
        return NULL;
    }

    ddscat_parser->line_number = 0;
    ddscat_parser->eof = false;
    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_OK;
    ddscat_parser->line[0] = '\0';
    ddscat_parser->err_msg[0] = '\0';

    return ddscat_parser;
}

void detscat_ddscat_parser_free(DetScatDdscatParser *ddscat_parser) {
    if (!ddscat_parser) return;

    if (ddscat_parser->file) {
        fclose(ddscat_parser->file);
        ddscat_parser->file = NULL;
    }

    free(ddscat_parser);
    return;
}

bool detscat_ddscat_parser_reset(DetScatDdscatParser *ddscat_parser,
                                 const char *ddscat_file_path) {
    assert(ddscat_parser != NULL);

    if (ddscat_parser->file) {
        fclose(ddscat_parser->file);
        ddscat_parser->file = NULL;
    }

    ddscat_parser->file = fopen(ddscat_file_path, "r");
    if (!ddscat_parser->file) {
        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_RESET;
        ddscat_parser->line_number = 0;
        ddscat_parser->eof = false;
        ddscat_parser->line[0] = '\0';
        snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                 "Parser reset failed. Could not open file: %s",
                 ddscat_file_path);
        return false;
    }

    ddscat_parser->line_number = 0;
    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_OK;
    ddscat_parser->eof = false;
    ddscat_parser->line[0] = '\0';
    ddscat_parser->err_msg[0] = '\0';
    return true;
}

bool detscat_ddscat_parser_parse_par(DetScatDdscatParser *ddscat_parser,
                                     DetScatDdscatParams *par) {
    assert(ddscat_parser != NULL);
    assert(par != NULL);

    size_t components_allocated = 0;
    size_t scat_planes_parsed = 0;

    enum DdscatParParseState {
        PARSE_INITIAL,
        PARSE_COMP,
        PARSE_PLANES
    };
    enum DdscatParParseState state = PARSE_INITIAL;

    while (fgets(ddscat_parser->line, DETSCAT_DDSCAT_LINE_MAX,
                 ddscat_parser->file)) {
        ddscat_parser->line_number++;
        char *trimmed = strutil_trim(ddscat_parser->line);

        switch (state) {
            case PARSE_INITIAL:
                if (strstr(trimmed, "NCOMP")) {
                    size_t n_components;
                    if (sscanf(trimmed, "%zu", &n_components) != 1 ||
                        n_components == 0) {
                        snprintf(ddscat_parser->err_msg,
                                 sizeof(ddscat_parser->err_msg),
                                 "Invalid format or zero NCOMP at line %d",
                                 ddscat_parser->line_number);
                        ddscat_parser->status =
                            DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                        goto cleanup;
                    }

                    par->n_components = n_components;
                    par->components = calloc(n_components, sizeof(char *));
                    if (!par->components) {
                        snprintf(ddscat_parser->err_msg,
                                 sizeof(ddscat_parser->err_msg),
                                 "Failed allocation at line %d",
                                 ddscat_parser->line_number);
                        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
                        goto cleanup;
                    }

                    components_allocated = 0;
                    state = PARSE_COMP;
                } else if (strstr(trimmed, "NPLANES")) {
                    size_t n_scat_planes;
                    if (sscanf(trimmed, "%zu", &n_scat_planes) != 1 ||
                        n_scat_planes == 0) {
                        snprintf(ddscat_parser->err_msg,
                                 sizeof(ddscat_parser->err_msg),
                                 "Invalid format or zero NPLANES at line %d",
                                 ddscat_parser->line_number);
                        ddscat_parser->status =
                            DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                        goto cleanup;
                    }

                    par->n_scat_planes = n_scat_planes;
                    par->scat_planes = calloc(
                        n_scat_planes,
                        sizeof(double[DETSCAT_DDSCAT_SCAT_PLANE_PARAMS]));
                    if (!par->scat_planes) {
                        snprintf(ddscat_parser->err_msg,
                                 sizeof(ddscat_parser->err_msg),
                                 "Failed allocation at line %d",
                                 ddscat_parser->line_number);
                        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
                        goto cleanup;
                    }

                    scat_planes_parsed = 0;
                    state = PARSE_PLANES;
                } else if (strstr(trimmed, "Polarization state")) {
                    if (sscanf(trimmed, "(%lf, %lf) (%lf, %lf) (%lf, %lf)",
                               &par->e01.x.re, &par->e01.x.im, &par->e01.y.re,
                               &par->e01.y.im, &par->e01.z.re,
                               &par->e01.z.im) != 6) {
                        snprintf(ddscat_parser->err_msg,
                                 sizeof(ddscat_parser->err_msg),
                                 "Invalid format for polarization state at line %d",
                                 ddscat_parser->line_number);
                        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                        goto cleanup;
                    }
                    state = PARSE_INITIAL;
                }
                break;

            case PARSE_COMP:
                if (components_allocated >= par->n_components) {
                    snprintf(ddscat_parser->err_msg,
                             sizeof(ddscat_parser->err_msg),
                             "Too many components provided (expected %zu) at "
                             "line %d",
                             par->n_components, ddscat_parser->line_number);
                    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                    goto cleanup;
                }

                char component[DETSCAT_DDSCAT_COMPONENTS_MAX];

                if (sscanf(trimmed, "'%[^']'", component) != 1) {
                    snprintf(ddscat_parser->err_msg,
                             sizeof(ddscat_parser->err_msg),
                             "Invalid format for component at line %d",
                             ddscat_parser->line_number);
                    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                    goto cleanup;
                }

                par->components[components_allocated] = strdup(component);
                if (!par->components[components_allocated]) {
                    snprintf(ddscat_parser->err_msg,
                             sizeof(ddscat_parser->err_msg),
                             "Failed component allocation at line %d",
                             ddscat_parser->line_number);
                    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
                    goto cleanup;
                }

                components_allocated++;
                if (components_allocated == par->n_components) {
                    state = PARSE_INITIAL;
                }
                break;

            case PARSE_PLANES:
                if (scat_planes_parsed >= par->n_scat_planes) {
                    snprintf(ddscat_parser->err_msg,
                             sizeof(ddscat_parser->err_msg),
                             "Too many scattering planes provided (expected "
                             "%zu) at line %d",
                             par->n_scat_planes, ddscat_parser->line_number);
                    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                    goto cleanup;
                }

                if (sscanf(trimmed, "%lf %lf %lf %lf",
                           &par->scat_planes[scat_planes_parsed][0],
                           &par->scat_planes[scat_planes_parsed][1],
                           &par->scat_planes[scat_planes_parsed][2],
                           &par->scat_planes[scat_planes_parsed][3]) !=
                    DETSCAT_DDSCAT_SCAT_PLANE_PARAMS) {
                    snprintf(ddscat_parser->err_msg,
                             sizeof(ddscat_parser->err_msg),
                             "Invalid format for scattering plane at line %d",
                             ddscat_parser->line_number);
                    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                    goto cleanup;
                }

                scat_planes_parsed++;
                if (scat_planes_parsed == par->n_scat_planes) {
                    state = PARSE_INITIAL;
                }
                break;
        }
    }

    if (components_allocated != par->n_components) {
        snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                 "Expected %zu components but got %zu", par->n_components,
                 components_allocated);
        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
        goto cleanup;
    }
    if (scat_planes_parsed != par->n_scat_planes) {
        snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                 "Expected %zu scattering planes but got %zu",
                 par->n_scat_planes, scat_planes_parsed);
        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
        goto cleanup;
    }

    ddscat_parser->eof = true;
    return true;

cleanup:
    if (par->components) {
        for (size_t i = 0; i < components_allocated; ++i) {
            free(par->components[i]);
            par->components[i] = NULL;
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

    return false;
}

bool detscat_ddscat_parser_parse_fml(DetScatDdscatParser *ddscat_parser,
                                     DetScatDdscatFml *fml,
                                     DetScatDdscatParams *par) {
    assert(ddscat_parser != NULL);
    assert(fml != NULL);

    fml->n_fmats = par->n_scat_planes;

    if (!(fml->n_fmats > 0)) {
        snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                 "Number of scattering planes must be larger than zero, "
                 "received %zu.",
                 fml->n_fmats);
        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
        goto cleanup;
    }

    size_t matrices_allocated = 0;

    bool data_header_found = false;
    while (fgets(ddscat_parser->line, DETSCAT_DDSCAT_LINE_MAX,
                 ddscat_parser->file)) {
        ddscat_parser->line_number++;
        char *trimmed = strutil_trim(ddscat_parser->line);

        if (strstr(trimmed, "Re")) {
            data_header_found = true;
            break;
        }
    }
    if (!data_header_found) {
        snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                 "Reached EOF. Did not find any data header.");
        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
        goto cleanup;
    }

    fml->fmats = calloc(fml->n_fmats, sizeof(DetScatDdscatFmatrix));
    if (!fml->fmats) {
        snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                 "Failed allocation for f matrices at line number %d.",
                 ddscat_parser->line_number);
        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
        goto cleanup;
    }

    for (size_t i = 0; i < fml->n_fmats; ++i) {
        size_t n_theta;

        double range = par->scat_planes[i][2] - par->scat_planes[i][1];
        double step = par->scat_planes[i][3];

        if (step <= 0.0) {
            snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                     "Division by zero encountered while calculating n_theta "
                     "for scattering plane %zu.",
                     i + 1);
            ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
            goto cleanup;
        } else if (range < 0.0) {
            snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                     "Invalid range encountered while calculating n_theta for "
                     "scattering plane %zu.",
                     i + 1);
            ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
            goto cleanup;
        } else {
            n_theta = (size_t)(range / step) + 1;
        }

        fml->fmats[i].n_theta = n_theta;

        fml->fmats[i].phi = par->scat_planes[i][0];

        fml->fmats[i].theta = calloc(n_theta, sizeof(double));
        fml->fmats[i].f11 = calloc(n_theta, sizeof(Complex));
        fml->fmats[i].f12 = calloc(n_theta, sizeof(Complex));
        fml->fmats[i].f21 = calloc(n_theta, sizeof(Complex));
        fml->fmats[i].f22 = calloc(n_theta, sizeof(Complex));

        if (!fml->fmats[i].theta || !fml->fmats[i].f11 ||
            !fml->fmats[i].f12 || !fml->fmats[i].f21 ||
            !fml->fmats[i].f22) {
            snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                     "Failed allocation of f matrix elements for scattering "
                     "plane %zu.",
                     i + 1);
            ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
            goto cleanup;
        }

        for (size_t j = 0; j < n_theta; ++j) {
            ddscat_parser->line_number++;

            if (!fgets(ddscat_parser->line, DETSCAT_DDSCAT_LINE_MAX,
                       ddscat_parser->file)) {
                snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                         "Could not read line %d.", ddscat_parser->line_number);
                ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                goto cleanup;
            }

            char *trimmed = strutil_trim(ddscat_parser->line);
            if (sscanf(trimmed, "%lf %*f %lf %lf %lf %lf %lf %lf %lf %lf",
                       &fml->fmats[i].theta[j], &fml->fmats[i].f11[j].re,
                       &fml->fmats[i].f11[j].im, &fml->fmats[i].f21[j].re,
                       &fml->fmats[i].f21[j].im, &fml->fmats[i].f12[j].re,
                       &fml->fmats[i].f12[j].im, &fml->fmats[i].f22[j].re,
                       &fml->fmats[i].f22[j].im) != 9) {
                snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                         "Could not parse line %d.",
                         ddscat_parser->line_number);
                ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                goto cleanup;
            }
        }

        matrices_allocated++;
    }
    while (fgets(ddscat_parser->line, DETSCAT_DDSCAT_LINE_MAX,
                 ddscat_parser->file)) {
        continue;
    }
    ddscat_parser->eof = true;
    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_OK;
    return true;

cleanup:
    if (fml->fmats) {
        for (size_t i = 0; i < matrices_allocated; ++i) {
            free(fml->fmats[i].theta);
            free(fml->fmats[i].f11);
            free(fml->fmats[i].f21);
            free(fml->fmats[i].f12);
            free(fml->fmats[i].f22);
            fml->fmats[i].theta = NULL;
            fml->fmats[i].f11 = NULL;
            fml->fmats[i].f21 = NULL;
            fml->fmats[i].f12 = NULL;
            fml->fmats[i].f22 = NULL;

            fml->fmats[i].phi = 0;
            fml->fmats[i].n_theta = 0;
        }

        free(fml->fmats);
        fml->fmats = NULL;

        fml->n_fmats = 0;
    }

    return false;
}

void detscat_ddscat_par_free(DetScatDdscatParams *par) {
    if (!par) return;

    if (par->components) {
        for (size_t i = 0; i < par->n_components; ++i) {
            free(par->components[i]);
            par->components[i] = NULL;
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

    return;
}

void detscat_ddscat_fml_free(DetScatDdscatFml *fml) {
    if (!fml) return;

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

    return;
}

void detscat_ddscat_data_free(DetScatDdscatData *ddscat) {
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

    if (ddscat->par_idx) {
        free(ddscat->par_idx);
        ddscat->par_idx = NULL;
    }

    ddscat->n_pars = 0;
    ddscat->n_fmls = 0;
    ddscat->n_par_idx = 0;

    return;
}
