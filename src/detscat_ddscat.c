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


DetScatDdscatParser *detscat_ddscat_parser_create(const char *ddscat_file_path) {
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


bool detscat_ddscat_parser_parse_par(DetScatDdscatParser *ddscat_parser, DetScatDdscatParams *par) {
    assert(ddscat_parser != NULL);
    assert(par != NULL);

    size_t components_allocated;
    size_t scat_planes_allocated;

    enum DdscatParParseState { PARSE_INITIAL, PARSE_COMP, PARSE_PLANES, PARSE_POLARIZATION };
    enum DdscatParParseState state = PARSE_INITIAL;

    while (fgets(ddscat_parser->line, DETSCAT_DDSCAT_LINE_MAX, ddscat_parser->file)) {
        ddscat_parser->line_number++;

        char *trimmed = strutil_trim(ddscat_parser->line);

        switch (state) {
            case PARSE_INITIAL:
                if (strstr(trimmed, "NCOMP")) {
                    size_t n_components;
                    if (sscanf(trimmed, "%zu", &n_components) != 1
                        || n_components == 0) {
                        snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                                 "Invalid format or zero NCOMP at line %d",
                                 ddscat_parser->line_number);
                        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                        goto cleanup;
                    }

                    par->n_components = n_components;
                    par->components = calloc(n_components, sizeof(char *));
                    if (!par->components) {
                        snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
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
                        snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                                 "Invalid format or zero NPLANES at line %d",
                                 ddscat_parser->line_number);
                        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                        goto cleanup;
                    }

                    par->n_scat_planes = n_scat_planes;
                    par->scat_planes = calloc(n_scat_planes, sizeof(double[DETSCAT_DDSCAT_SCAT_PLANE_PARAMS]));
                    if (!par->scat_planes) {
                        snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                                 "Failed allocation at line %d",
                                 ddscat_parser->line_number);
                        ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
                        goto cleanup;
                    }

                    scat_planes_allocated = 0;
                    state = PARSE_PLANES;
                } else if (strstr(trimmed, "Polarization state")) {
                    state = PARSE_POLARIZATION;
                }
                break;

            case PARSE_COMP:
                if (components_allocated >= par->n_components) {
                    state = PARSE_INITIAL;
                    break;
                }

                par->components[components_allocated] = malloc(DETSCAT_DDSCAT_COMPONENTS_MAX * sizeof(char));
                if (!par->components[components_allocated]) {
                    snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                             "Failed component allocation at line %d",
                             ddscat_parser->line_number);
                    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
                    goto cleanup;
                }
                components_allocated++;

                if (sscanf(trimmed, "'%[^']'", par->components[components_allocated]) != 1) {
                    snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                             "Invalid format for component at line %d",
                             ddscat_parser->line_number);
                    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                    goto cleanup;
                }
                break;

            case PARSE_PLANES:
                if (scat_planes_allocated >= par->n_scat_planes) {
                    state = PARSE_INITIAL;
                    break;
                }

                if (sscanf(trimmed, "%lf %lf %lf %lf",
                           &par->scat_planes[scat_planes_allocated][0],
                           &par->scat_planes[scat_planes_allocated][1],
                           &par->scat_planes[scat_planes_allocated][2],
                           &par->scat_planes[scat_planes_allocated][3]) != DETSCAT_DDSCAT_SCAT_PLANE_PARAMS) {
                    snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                             "Invalid format for scattering plane at line %d",
                             ddscat_parser->line_number);
                    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                    goto cleanup;
                }

                planes_parsed++;
                if (planes_parsed == par->nplanes) {
                    state = PARSE_INITIAL;
                }
                break;

            case PARSE_POLARIZATION:
                if (sscanf(trimmed, "(%lf, %lf) (%lf, %lf) (%lf, %lf)",
                           &par->e01.x.re, &par->e01.x.im, &par->e01.y.re,
                           &par->e01.y.im, &par->e01.z.re,
                           &par->e01.z.im) != 6) {
                    snprintf(ddscat_parser->err_msg, sizeof(ddscat_parser->err_msg),
                             "Invalid format for polarization state at line %d",
                             ddscat_parser->line_number);
                    ddscat_parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
                    goto cleanup;
                }
                state = PARSE_INITIAL;
                break;
        }
    }
    goto cleanup;

cleanup:
    fclose(ddscat_parser->file);
    ddscat_parser->file = NULL;

    if (status != DETSCAT_DDSCAT_UTIL_OK) {
        if (par->comp) {
            for (size_t i = 0; i < comp_allocated; ++i) {
                free(par->comp[i]);
            }
            free(par->comp);
            par->comp = NULL;
        }

        if (par->planes) {
            free(par->planes);
            par->planes = NULL;
        }

        par->ncomp = 0;
        par->nplanes = 0;
    }

    return status;
}

// DetScatDdscatUtilStatus detscat_ddscat_util_parse_fml_file(const char *fml_file_path, const DdscatPar *par,
//                                   Fmat **fmat) {
//     FILE *fml_file = fopen(fml_file_path, "r");
//     if (!fml_file) return DETSCAT_DDSCAT_UTIL_ERR_CANNOT_OPEN_FILE;

//     char line[DETSCAT_DDSCAT_UTIL_LINE_MAX];
//     DetScatDdscatUtilStatus status = DETSCAT_DDSCAT_UTIL_OK;

//     size_t fmat_allocated = 0;

//     while (fgets(line, DETSCAT_DDSCAT_UTIL_LINE_MAX, fml_file)) {
//         if (strstr(line, "Re")) {
//             break;
//         }
//     }

//     *fmat = (Fmat *)malloc(par->nplanes * sizeof(Fmat));
//     if (!*fmat) {
//         status = DETSCAT_DDSCAT_UTIL_ERR_ALLOC;
//         goto cleanup;
//     }
//     for (size_t i = 0; i < par->nplanes; ++i) {
//         // TODO: Add some safety checks before casting
//         size_t n_theta = (size_t)((par->planes[i][2] - par->planes[i][1]) /
//                                   par->planes[i][3]) +
//                          1;

//         (*fmat)[i].phi = par->planes[i][0];
//         (*fmat)[i].n = n_theta;

//         (*fmat)[i].theta = NULL;

//         (*fmat)[i].f11 = (*fmat)[i].f12 = (*fmat)[i].f21 = (*fmat)[i].f22 =
//             NULL;
//         (*fmat)[i].theta = malloc(n_theta * sizeof(double));
//         (*fmat)[i].f11 = malloc(n_theta * sizeof(Complex));
//         (*fmat)[i].f12 = malloc(n_theta * sizeof(Complex));
//         (*fmat)[i].f21 = malloc(n_theta * sizeof(Complex));
//         (*fmat)[i].f22 = malloc(n_theta * sizeof(Complex));

//         if (!(*fmat)[i].theta || !(*fmat)[i].f11 || !(*fmat)[i].f12 || !(*fmat)[i].f21 ||
//             !(*fmat)[i].f22) {
//             status = DETSCAT_DDSCAT_UTIL_ERR_ALLOC;
//             goto cleanup;
//         }

//         fmat_allocated++;

//         for (size_t j = 0; j < n_theta; ++j) {
//             if (!fgets(line, DETSCAT_DDSCAT_UTIL_LINE_MAX, fml_file)) {
//                 status = DETSCAT_DDSCAT_UTIL_ERR_PARSING;
//                 goto cleanup;
//             }
//             if (sscanf(line, "%lf %*f %lf %lf %lf %lf %lf %lf %lf %lf",
//                        &(*fmat)[i].theta[j],
//                        &(*fmat)[i].f11[j].re, &(*fmat)[i].f11[j].im,
//                        &(*fmat)[i].f21[j].re, &(*fmat)[i].f21[j].im,
//                        &(*fmat)[i].f12[j].re, &(*fmat)[i].f12[j].im,
//                        &(*fmat)[i].f22[j].re, &(*fmat)[i].f22[j].im) != 9) {
//                 status = DETSCAT_DDSCAT_UTIL_ERR_PARSING;
//                 goto cleanup;
//             }
//         }
//     }
//     fclose(fml_file);
//     return status;

// cleanup:
//     fclose(fml_file);

//     if (*fmat) {
//         for (size_t i = 0; i < fmat_allocated; ++i) {
//             free((*fmat)[i].theta);
//             free((*fmat)[i].f11);
//             free((*fmat)[i].f21);
//             free((*fmat)[i].f12);
//             free((*fmat)[i].f22);
//         }
//         free(*fmat);
//         *fmat = NULL;
//     }

//     return status;
// }
