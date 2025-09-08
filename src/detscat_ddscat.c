#include "detscat_ddscat.h"

#include "detscat.h"

#include "detscat_error.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>




// #include "detscat_parser.h"
// #include <assert.h>
// #include <errno.h>
// #include <stdbool.h>
// #include <stdint.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>

// #include "detscat_math.h"
// #include "detscat_str.h"

// static const size_t DETSCAT_DDSCAT_PATH_INIT = 512;
// static const size_t DETSCAT_DDSCAT_PATH_MAX = 4096;
// static const size_t DETSCAT_DDSCAT_SCAT_PLANE_PARAMS = 4;

// typedef enum {
//     PARSE_INITIAL = 0,
//     PARSE_COMP,
//     PARSE_PLANES 
// } DdscatParParserState;

// typedef struct {
//     DdscatParParserState state;
//     size_t components_allocated;
//     size_t scat_planes_parsed;
// } DdscatParParserContext;

// static void detscat_ddscat_par_free_count(DetScatDdscatParams *par, size_t count) {
//     if (!par) return;

//     if (par->components) {
//         for (size_t i = 0; i < count; ++i) {
//             detscat_str_free(&par->components[i]);
//         }
//         free(par->components);
//         par->components = NULL;
//     }

//     if (par->scat_planes) {
//         free(par->scat_planes);
//         par->scat_planes = NULL;
//     }

//     par->n_components = 0;
//     par->n_scat_planes = 0;
//     memset(&par->e01, 0, sizeof(par->e01));
// }



// static bool detscat_ddscat_parse_par_component(DetScatParser *parser, Str *component, const char *line) {
//     assert(parser != NULL);
//     assert(component != NULL);

//     if (!line || !*line) goto invalid; 

//     const char *start = strchr(line, '\'');
//     if (!start) goto invalid; 
//     start++;

//     const char *end = strchr(start, '\''); 
//     if (!end) goto invalid; 

//     size_t len = end - start;

//     if (len >= DETSCAT_DDSCAT_PATH_MAX) {
//         parser->status = DETSCAT_PARSER_ERR_RANGE;
//         snprintf(parser->errmsg, sizeof(parser->errmsg),
//                  "Component data path out of range at line %d",
//                  parser->lineno);
//         return false;
//     }

//     char *path = malloc(len+1);
//     if (!path) {
//         parser->status = DETSCAT_PARSER_ERR_ALLOC;
//         snprintf(parser->errmsg, sizeof(parser->errmsg),
//                  "Could not allocate memory for temporary path at line %d",
//                  parser->lineno);
//         return false;
//     }
//     memcpy(path, start, len);
//     path[len] = '\0';

//     if (!str_set(component, path)) {
//         parser->status = DETSCAT_PARSER_ERR_ALLOC;
//         snprintf(parser->errmsg, sizeof(parser->errmsg),
//                  "Could not set component path at line %d",
//                  parser->lineno);
//         free(path);
//         return false;
//     }
//     str_raw_normpath(component->data);

//     free(path);
//     return true;

// invalid:
//     parser->status = DETSCAT_PARSER_ERR_FORMAT;
//     snprintf(parser->errmsg, sizeof(parser->errmsg),
//              "Invalid component at line %d", parser->lineno);
//     return false;

// }







// bool detscat_ddscat_fml_load(const char *file_path, DetScatDdscatFml *fml, 
//                              DetScatDdscatParams *par, DetScatDiagnose *diag) {
//     assert(fml != NULL);
//     assert(par != NULL);
//     assert(diag != NULL);

//     if (!file_path || !*file_path) {
//         DETSCAT_SET_DIAGNOSE(diag, DETSCAT_ERR_INVALID_ARG, "%s",
//                              "Function argument 'file_path' is invalid");
//         return false;
//     }

//     DetScatParser parser;
//     if (!detscat_parser_init(&parser, file_path)) {
//         DETSCAT_SET_DIAGNOSE(diag, DETSCAT_ERR_PARSING,
//                              "Could not initialize parser: %s", parser.errmsg);
//         return false;
//     }

//     size_t matrices_allocated = 0;

//     fml->n_fmats = par->n_scat_planes;

//     if (fml->n_fmats <= 0) {
//         snprintf(parser->errmsg, sizeof(parser->errmsg),
//                  "Number of scattering planes must be larger than zero, "
//                  "got %zu",
//                  fml->n_fmats);
//         parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
//         goto cleanup;
//     }

//     bool data_header_found = false;
//     while (fgets(parser->line, DETSCAT_DDSCAT_LINE_MAX, parser->file)) {
//         parser->line_number++;
//         char *trimmed = strutil_trim(parser->line);

//         if (strstr(trimmed, "Re")) {
//             data_header_found = true;
//             break;
//         }
//     }
//     if (!data_header_found) {
//         snprintf(parser->errmsg, sizeof(parser->errmsg),
//                  "Reached EOF. Did not find any data header.");
//         parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
//         goto cleanup;
//     }

//     fml->fmats = calloc(fml->n_fmats, sizeof(DetScatDdscatFmatrix));
//     if (!fml->fmats) {
//         snprintf(parser->errmsg, sizeof(parser->errmsg),
//                  "Memory allocation failed for f matrices at line number %d",
//                  parser->line_number);
//         parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
//         goto cleanup;
//     }

//     for (size_t i = 0; i < fml->n_fmats; ++i) {
//         size_t n_theta;

//         double range = par->scat_planes[i][2] - par->scat_planes[i][1];
//         double step = par->scat_planes[i][3];

//         if (step <= 0.0) {
//             snprintf(parser->errmsg, sizeof(parser->errmsg),
//                      "Division by zero encountered while calculating n_theta "
//                      "for scattering plane %zu",
//                      i + 1);
//             parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
//             goto cleanup;
//         } else if (range < 0.0) {
//             snprintf(parser->errmsg, sizeof(parser->errmsg),
//                      "Invalid range encountered while calculating n_theta for "
//                      "scattering plane %zu.",
//                      i + 1);
//             parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
//             goto cleanup;
//         } else {
//             n_theta = (size_t)(range / step) + 1;
//         }

//         fml->fmats[i].n_theta = n_theta;

//         fml->fmats[i].phi = par->scat_planes[i][0];

//         fml->fmats[i].theta = calloc(n_theta, sizeof(double));
//         fml->fmats[i].f11 = calloc(n_theta, sizeof(Complex));
//         fml->fmats[i].f12 = calloc(n_theta, sizeof(Complex));
//         fml->fmats[i].f21 = calloc(n_theta, sizeof(Complex));
//         fml->fmats[i].f22 = calloc(n_theta, sizeof(Complex));

//         if (!fml->fmats[i].theta || !fml->fmats[i].f11 || !fml->fmats[i].f12 ||
//             !fml->fmats[i].f21 || !fml->fmats[i].f22) {
//             snprintf(
//                 parser->errmsg, sizeof(parser->errmsg),
//                 "Memory allocation failed for f matrix elements for scattering "
//                 "plane %zu",
//                 i + 1);
//             parser->status = DETSCAT_DDSCAT_PARSER_ERR_ALLOC;
//             goto cleanup;
//         }

//         for (size_t j = 0; j < n_theta; ++j) {
//             parser->line_number++;

//             if (!fgets(parser->line, DETSCAT_DDSCAT_LINE_MAX, parser->file)) {
//                 snprintf(parser->errmsg, sizeof(parser->errmsg),
//                          "Could not read line %d", parser->line_number);
//                 parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
//                 goto cleanup;
//             }

//             char *trimmed = strutil_trim(parser->line);
//             if (sscanf(trimmed, "%lf %*f %lf %lf %lf %lf %lf %lf %lf %lf",
//                        &fml->fmats[i].theta[j], &fml->fmats[i].f11[j].re,
//                        &fml->fmats[i].f11[j].im, &fml->fmats[i].f21[j].re,
//                        &fml->fmats[i].f21[j].im, &fml->fmats[i].f12[j].re,
//                        &fml->fmats[i].f12[j].im, &fml->fmats[i].f22[j].re,
//                        &fml->fmats[i].f22[j].im) != 9) {
//                 snprintf(parser->errmsg, sizeof(parser->errmsg),
//                          "Could not parse line %d", parser->line_number);
//                 parser->status = DETSCAT_DDSCAT_PARSER_ERR_FORMAT;
//                 goto cleanup;
//             }
//         }

//         matrices_allocated++;
//     }
//     while (fgets(parser->line, DETSCAT_DDSCAT_LINE_MAX, parser->file)) {
//         continue;
//     }
//     parser->eof = true;
//     parser->status = DETSCAT_DDSCAT_PARSER_OK;
//     return true;

// cleanup:
//     if (fml->fmats) {
//         for (size_t i = 0; i < matrices_allocated; ++i) {
//             free(fml->fmats[i].theta);
//             free(fml->fmats[i].f11);
//             free(fml->fmats[i].f21);
//             free(fml->fmats[i].f12);
//             free(fml->fmats[i].f22);
//             fml->fmats[i].theta = NULL;
//             fml->fmats[i].f11 = NULL;
//             fml->fmats[i].f21 = NULL;
//             fml->fmats[i].f12 = NULL;
//             fml->fmats[i].f22 = NULL;

//             fml->fmats[i].phi = 0;
//             fml->fmats[i].n_theta = 0;
//         }

//         free(fml->fmats);
//         fml->fmats = NULL;

//         fml->n_fmats = 0;
//     }

//     return false;
// }









// --- Internal helpers (SHARED)
DetScatDdscat *detscat_ddscat_create(size_t n_pars, size_t n_fmls, size_t n_par_idxs, DetScatError *err) {
    if (!n_pars || !n_fmls || !n_par_idxs) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_INVALID_ARG,
                          "Invalid or zero size parameters for ddscat provided.");
        return NULL;
    }

    DetScatDdscat *ddscat = calloc(1, sizeof(*ddscat));
    if (!ddscat) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Could not allocate memory for ddscat object");
        return NULL;
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
        free(ddscat);
        return NULL;
    }

    ddscat->n_pars = n_pars;
    ddscat->n_fmls = n_fmls;
    ddscat->n_par_idxs = n_par_idxs;

    return ddscat;
}

void detscat_ddscat_par_clear(DetScatDdscatParams *par) {
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

void detscat_ddscat_fml_clear(DetScatDdscatFml *fml) {
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

void detscat_ddscat_destroy(DetScatDdscat **ddscat) {
    if (!ddscat || !*ddscat) return;

    if ((*ddscat)->pars) {
        for (size_t i = 0; i < (*ddscat)->n_pars; ++i) {
            detscat_ddscat_par_clear(&(*ddscat)->pars[i]);
        }
        free((*ddscat)->pars);
        (*ddscat)->pars = NULL;
    }

    if ((*ddscat)->fmls) {
        for (size_t i = 0; i < (*ddscat)->n_fmls; ++i) {
            detscat_ddscat_fml_clear(&(*ddscat)->fmls[i]);
        }
        free((*ddscat)->fmls);
        (*ddscat)->fmls = NULL;
    }

    if ((*ddscat)->par_idxs) {
        free((*ddscat)->par_idxs);
        (*ddscat)->par_idxs = NULL;
    }

    (*ddscat)->n_pars = 0;
    (*ddscat)->n_fmls = 0;
    (*ddscat)->n_par_idxs = 0;

    free(*ddscat);
    *ddscat = NULL;
}
