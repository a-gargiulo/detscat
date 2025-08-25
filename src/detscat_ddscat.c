#include "detscat_ddscat.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mymath.h"
#include "strutil.h"

bool detscat_ddscat_parser_init(DetScatDdscatParser *parser,
                                const char *file_path) {
    assert(parser != NULL);
    assert(file_path != NULL && file_path[0] != '\0');

    parser->file = fopen(file_path, "r");
    if (!parser->file) return false;

    parser->line_number = 0;
    parser->eof = false;
    parser->status = DETSCAT_DDSCAT_PARSER_OK;
    parser->line[0] = '\0';
    parser->errmsg[0] = '\0';

    return true;
}

void detscat_ddscat_parser_close(DetScatDdscatParser *parser) {
    assert(parser != NULL);

    if (parser->file) {
        fclose(parser->file);
        parser->file = NULL;
    }

    parser->line_number = 0;
    parser->status = DETSCAT_DDSCAT_PARSER_OK;
    parser->eof = false;
    parser->line[0] = '\0';
    parser->errmsg[0] = '\0';
}

bool detscat_ddscat_parser_reset(DetScatDdscatParser *parser,
                                 const char *file_path) {
    assert(parser != NULL);
    assert(file_path != NULL && file_path[0] != '\0');

    if (parser->file) {
        fclose(parser->file);
        parser->file = NULL;
    }

    parser->file = fopen(file_path, "r");
    if (!parser->file) {
        parser->status = DETSCAT_DDSCAT_PARSER_ERR_RESET;
        parser->line_number = 0;
        parser->eof = false;
        parser->line[0] = '\0';
        parser->errmsg[0] = '\0';
        return false;
    }

    parser->line_number = 0;
    parser->status = DETSCAT_DDSCAT_PARSER_OK;
    parser->eof = false;
    parser->line[0] = '\0';
    parser->errmsg[0] = '\0';
    
    return true;
}

bool detscat_ddscat_init(DetScatDdscatData *ddscat,
                         size_t n_pars,
                         size_t n_fmls,
                         size_t n_par_idxs) {

    ddscat->pars = n_pars ? calloc(n_pars, sizeof(*ddscat->pars)) : NULL;
    ddscat->fmls = n_fmls ? calloc(n_fmls, sizeof(*ddscat->fmls)) : NULL;
    ddscat->par_idxs = n_par_idxs ? calloc(n_par_idxs, sizeof(*ddscat->par_idxs)) : NULL;

    ddscat->n_pars = n_pars;
    ddscat->n_fmls = n_fmls;
    ddscat->n_par_idxs = n_par_idxs;

    if ((n_pars && !ddscat->pars) ||
        (n_fmls && !ddscat->fmls) ||
        (n_par_idxs && !ddscat->par_idxs)) 
    {
        free(ddscat->pars);
        free(ddscat->fmls);
        free(ddscat->par_idxs);
        ddscat->pars = NULL;
        ddscat->fmls = NULL;
        ddscat->par_idxs = NULL;
        ddscat->n_pars = ddscat->n_fmls = ddscat->n_par_idxs = 0;
        return false;
    }

    return true;

}

bool detscat_ddscat_parser_par_load(DetScatDdscatParser *parser,
                                    DetScatDdscatParams *par) {
    assert(parser != NULL);
    assert(par != NULL);

    size_t components_allocated = 0;
    size_t scat_planes_parsed = 0;

    enum DdscatParParseState {
        PARSE_INITIAL,
        PARSE_COMP,
        PARSE_PLANES
    };
    enum DdscatParParseState state = PARSE_INITIAL;

    while (fgets(parser->line, DETSCAT_DDSCAT_LINE_MAX, parser->file)) {
        parser->line_number++;
        char *trimmed = strutil_trim(parser->line);

        switch (state) {
            case PARSE_INITIAL:
                if (strstr(trimmed, "NCOMP")) {
                    size_t n_components;
                    if (sscanf(trimmed, "%zu", &n_components) != 1 ||
                        n_components == 0) {
                        snprintf(parser->errmsg, sizeof(parser->errmsg),
                                 "Invalid NCOMP format at line %d",
                                 parser->line_number);
                        parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                        goto cleanup;
                    }

                    par->n_components = n_components;
                    par->components = calloc(n_components, sizeof(char *));
                    if (!par->components) {
                        snprintf(parser->errmsg, sizeof(parser->errmsg),
                                 "Memory allocation for components failed");
                        parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
                        goto cleanup;
                    }

                    components_allocated = 0;
                    state = PARSE_COMP;
                } else if (strstr(trimmed, "NPLANES")) {
                    size_t n_scat_planes;
                    if (sscanf(trimmed, "%zu", &n_scat_planes) != 1 ||
                        n_scat_planes == 0) {
                        snprintf(parser->errmsg, sizeof(parser->errmsg),
                                 "Invalid NPLANES format at line %d",
                                 parser->line_number);
                        parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                        goto cleanup;
                    }

                    par->n_scat_planes = n_scat_planes;
                    par->scat_planes = calloc(
                        n_scat_planes,
                        sizeof(double[DETSCAT_DDSCAT_SCAT_PLANE_PARAMS]));
                    if (!par->scat_planes) {
                        snprintf(parser->errmsg, sizeof(parser->errmsg),
                                "Memory allocation for scattering planes "
                                "failed");
                        parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
                        goto cleanup;
                    }

                    scat_planes_parsed = 0;
                    state = PARSE_PLANES;
                } else if (strstr(trimmed, "Polarization state")) {
                    if (sscanf(trimmed, "(%lf, %lf) (%lf, %lf) (%lf, %lf)",
                               &par->e01.x.re, &par->e01.x.im, &par->e01.y.re,
                               &par->e01.y.im, &par->e01.z.re,
                               &par->e01.z.im) != 6) {
                        snprintf(parser->errmsg, sizeof(parser->errmsg),
                                 "Invalid format for polarization state at line %d",
                                 parser->line_number);
                        parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                        goto cleanup;
                    }
                    state = PARSE_INITIAL;
                }
                break;

            case PARSE_COMP:
                if (components_allocated >= par->n_components) {
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Too many components provided (expected %zu) at "
                             "line %d",
                             par->n_components, parser->line_number);
                    parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                    goto cleanup;
                }

                char component[DETSCAT_DDSCAT_COMPONENTS_MAX];

                if (sscanf(trimmed, "'%[^']'", component) != 1) {
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Invalid format for component at line %d",
                             parser->line_number);
                    parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                    goto cleanup;
                }

                par->components[components_allocated] = strutil_strdup(component);
                if (!par->components[components_allocated]) {
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Failed component allocation at line %d",
                             parser->line_number);
                    parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
                    goto cleanup;
                }

                components_allocated++;
                if (components_allocated == par->n_components) {
                    state = PARSE_INITIAL;
                }
                break;

            case PARSE_PLANES:
                if (scat_planes_parsed >= par->n_scat_planes) {
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Too many scattering planes provided (expected "
                             "%zu) at line %d",
                             par->n_scat_planes, parser->line_number);
                    parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                    goto cleanup;
                }

                if (sscanf(trimmed, "%lf %lf %lf %lf",
                           &par->scat_planes[scat_planes_parsed][0],
                           &par->scat_planes[scat_planes_parsed][1],
                           &par->scat_planes[scat_planes_parsed][2],
                           &par->scat_planes[scat_planes_parsed][3]) !=
                    DETSCAT_DDSCAT_SCAT_PLANE_PARAMS) {
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Invalid format for scattering plane at line %d",
                             parser->line_number);
                    parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
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
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Expected %zu components but got %zu",
                 par->n_components, components_allocated);
        parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
        goto cleanup;
    }
    if (scat_planes_parsed != par->n_scat_planes) {
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Expected %zu scattering planes but got %zu",
                 par->n_scat_planes, scat_planes_parsed);
        parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
        goto cleanup;
    }

    parser->eof = true;
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
    memset(&par->e01, 0, sizeof(par->e01));

    return false;
}

bool detscat_ddscat_parser_fml_load(DetScatDdscatParser *parser,
                                    DetScatDdscatFml *fml,
                                    DetScatDdscatParams *par) {
    assert(parser != NULL);
    assert(fml != NULL);
    assert(par != NULL);

    size_t matrices_allocated = 0;

    fml->n_fmats = par->n_scat_planes;

    if (fml->n_fmats <= 0) {
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Number of scattering planes must be larger than zero, "
                 "got %zu",
                 fml->n_fmats);
        parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
        goto cleanup;
    }

    bool data_header_found = false;
    while (fgets(parser->line, DETSCAT_DDSCAT_LINE_MAX, parser->file)) {
        parser->line_number++;
        char *trimmed = strutil_trim(parser->line);

        if (strstr(trimmed, "Re")) {
            data_header_found = true;
            break;
        }
    }
    if (!data_header_found) {
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Reached EOF. Did not find any data header.");
        parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
        goto cleanup;
    }

    fml->fmats = calloc(fml->n_fmats, sizeof(DetScatDdscatFmatrix));
    if (!fml->fmats) {
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Memory allocation failed for f matrices at line number %d",
                 parser->line_number);
        parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
        goto cleanup;
    }

    for (size_t i = 0; i < fml->n_fmats; ++i) {
        size_t n_theta;

        double range = par->scat_planes[i][2] - par->scat_planes[i][1];
        double step = par->scat_planes[i][3];

        if (step <= 0.0) {
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Division by zero encountered while calculating n_theta "
                     "for scattering plane %zu",
                     i + 1);
            parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
            goto cleanup;
        } else if (range < 0.0) {
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Invalid range encountered while calculating n_theta for "
                     "scattering plane %zu.",
                     i + 1);
            parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
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
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Memory allocation failed for f matrix elements for scattering "
                     "plane %zu",
                     i + 1);
            parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
            goto cleanup;
        }

        for (size_t j = 0; j < n_theta; ++j) {
            parser->line_number++;

            if (!fgets(parser->line, DETSCAT_DDSCAT_LINE_MAX, parser->file)) {
                snprintf(parser->errmsg, sizeof(parser->errmsg),
                         "Could not read line %d",
                         parser->line_number);
                parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                goto cleanup;
            }

            char *trimmed = strutil_trim(parser->line);
            if (sscanf(trimmed, "%lf %*f %lf %lf %lf %lf %lf %lf %lf %lf",
                       &fml->fmats[i].theta[j], &fml->fmats[i].f11[j].re,
                       &fml->fmats[i].f11[j].im, &fml->fmats[i].f21[j].re,
                       &fml->fmats[i].f21[j].im, &fml->fmats[i].f12[j].re,
                       &fml->fmats[i].f12[j].im, &fml->fmats[i].f22[j].re,
                       &fml->fmats[i].f22[j].im) != 9) {
                snprintf(parser->errmsg, sizeof(parser->errmsg),
                         "Could not parse line %d",
                         parser->line_number);
                parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                goto cleanup;
            }
        }

        matrices_allocated++;
    }
    while (fgets(parser->line, DETSCAT_DDSCAT_LINE_MAX, parser->file)) {
        continue;
    }
    parser->eof = true;
    parser->status = DETSCAT_DDSCAT_PARSER_OK;
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
}

void detscat_ddscat_free(DetScatDdscatData *ddscat) {
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
