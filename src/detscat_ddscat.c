#include "detscat_ddscat.h"

#include "detscat_parser.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "detscat_math.h"
#include "detscat_str.h"

static const size_t DETSCAT_DDSCAT_PATH_INIT = 512;
static const size_t DETSCAT_DDSCAT_PATH_MAX = 4096;
static const size_t DETSCAT_DDSCAT_SCAT_PLANE_PARAMS = 4;

typedef enum {
    PARSE_INITIAL = 0,
    PARSE_COMP,
    PARSE_PLANES 
} DdscatParParserState;

typedef struct {
    DdscatParParserState state;
    size_t components_allocated;
    size_t scat_planes_parsed;
} DdscatParParserContext;

static void detscat_ddscat_par_free_count(DetScatDdscatParams *par, size_t count) {
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



static bool detscat_ddscat_parse_par_component(DetScatParser *parser, Str *component, const char *line) {
    assert(parser != NULL);
    assert(component != NULL);

    if (!line || !*line) goto invalid; 

    const char *start = strchr(line, '\'');
    if (!start) goto invalid; 
    start++;

    const char *end = strchr(start, '\''); 
    if (!end) goto invalid; 

    size_t len = end - start;

    if (len >= DETSCAT_DDSCAT_PATH_MAX) {
        parser->status = DETSCAT_PARSER_ERR_RANGE;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Component data path out of range at line %d",
                 parser->lineno);
        return false;
    }

    char *path = malloc(len+1);
    if (!path) {
        parser->status = DETSCAT_PARSER_ERR_ALLOC;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Could not allocate memory for temporary path at line %d",
                 parser->lineno);
        return false;
    }
    memcpy(path, start, len);
    path[len] = '\0';

    if (!str_set(component, path)) {
        parser->status = DETSCAT_PARSER_ERR_ALLOC;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Could not set component path at line %d",
                 parser->lineno);
        free(path);
        return false;
    }
    str_raw_normpath(component->data);

    free(path);
    return true;

invalid:
    parser->status = DETSCAT_PARSER_ERR_FORMAT;
    snprintf(parser->errmsg, sizeof(parser->errmsg),
             "Invalid component at line %d", parser->lineno);
    return false;

}




bool detscat_ddscat_init(DetScatDdscat *ddscat, size_t n_pars, size_t n_fmls,
                         size_t n_par_idxs) {
    ddscat->pars = n_pars ? calloc(n_pars, sizeof(*ddscat->pars)) : NULL;
    ddscat->fmls = n_fmls ? calloc(n_fmls, sizeof(*ddscat->fmls)) : NULL;
    ddscat->par_idxs =
        n_par_idxs ? calloc(n_par_idxs, sizeof(*ddscat->par_idxs)) : NULL;

    ddscat->n_pars = n_pars;
    ddscat->n_fmls = n_fmls;
    ddscat->n_par_idxs = n_par_idxs;

    if ((n_pars && !ddscat->pars) || (n_fmls && !ddscat->fmls) ||
        (n_par_idxs && !ddscat->par_idxs)) {
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

static bool detscat_ddscat_parse_par_line(DetScatParser *parser,
                                          DetScatDdscatParams *par,
                                          DdscatParParserContext *ctx) { 

    char *trimmed = str_raw_trim(parser->line.data);

    switch (ctx->state) {
        case PARSE_INITIAL:
            if (strstr(trimmed, "NCOMP")) {
                size_t n_components;
                if (sscanf(trimmed, "%zu", &n_components) != 1 ||
                    n_components == 0) {
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Invalid NCOMP format at line %d",
                             parser->lineno);
                    parser->status = DETSCAT_PARSER_ERR_FORMAT;
                    goto error_cleanup;
                }

                par->n_components = n_components;
                par->components = calloc(n_components,
                                         sizeof(*par->components));
                if (!par->components) {
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Memory allocation for components failed");
                    parser->status = DETSCAT_PARSER_ERR_ALLOC;
                    goto error_cleanup;
                }

                ctx->state = PARSE_COMP;
            } else if (strstr(trimmed, "NPLANES")) {
                size_t n_scat_planes;
                if (sscanf(trimmed, "%zu", &n_scat_planes) != 1 ||
                    n_scat_planes == 0) {
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Invalid NPLANES format at line %d",
                             parser->lineno);
                    parser->status = DETSCAT_PARSER_ERR_FORMAT;
                    goto error_cleanup;
                }

                par->n_scat_planes = n_scat_planes;
                par->scat_planes = calloc(n_scat_planes,
                        sizeof(double[DETSCAT_DDSCAT_SCAT_PLANE_PARAMS]));
                if (!par->scat_planes) {
                    snprintf(parser->errmsg, sizeof(parser->errmsg),
                             "Memory allocation for scattering planes "
                             "failed");
                    parser->status = DETSCAT_PARSER_ERR_ALLOC;
                    goto error_cleanup;
                }

                ctx->state = PARSE_PLANES;
            } else if (strstr(trimmed, "Polarization state")) {
                if (sscanf(trimmed, "(%lf, %lf) (%lf, %lf) (%lf, %lf)",
                           &par->e01.x.re, &par->e01.x.im, &par->e01.y.re,
                           &par->e01.y.im, &par->e01.z.re,
                           &par->e01.z.im) != 6) {
                    snprintf(
                        parser->errmsg, sizeof(parser->errmsg),
                        "Invalid format for polarization state at line %d",
                        parser->lineno);
                    parser->status = DETSCAT_PARSER_ERR_FORMAT;
                    goto error_cleanup;
                }
                ctx->state = PARSE_INITIAL;
            }
            return true;

        case PARSE_COMP:
            if (ctx->components_allocated >= par->n_components) {
                snprintf(parser->errmsg, sizeof(parser->errmsg),
                         "Too many components provided (expected %zu) at "
                         "line %d",
                         par->n_components, parser->lineno);
                parser->status = DETSCAT_PARSER_ERR_FORMAT;
                goto error_cleanup;
            }

            if (!str_init(&par->components[ctx->components_allocated]) ||
                !str_reserve(&par->components[ctx->components_allocated],
                             DETSCAT_DDSCAT_PATH_INIT)) {
                parser->status = DETSCAT_PARSER_ERR_ALLOC;
                snprintf(parser->errmsg, sizeof(parser->errmsg),
                         "Memory allocation failed for component path "
                         "at line %d",
                         parser->lineno);
                goto error_cleanup;
            }

            if (!detscat_ddscat_parse_par_component(
                    parser, &par->components[ctx->components_allocated], trimmed)) {
                goto error_cleanup;
            }

            ctx->components_allocated++;
            if (ctx->components_allocated == par->n_components) {
                ctx->state = PARSE_INITIAL;
            }
            return true;

        case PARSE_PLANES:
            if (ctx->scat_planes_parsed >= par->n_scat_planes) {
                snprintf(parser->errmsg, sizeof(parser->errmsg),
                         "Too many scattering planes provided (expected "
                         "%zu) at line %d",
                         par->n_scat_planes, parser->lineno);
                parser->status = DETSCAT_PARSER_ERR_FORMAT;
                goto error_cleanup;
            }

            if (sscanf(trimmed, "%lf %lf %lf %lf",
                       &par->scat_planes[ctx->scat_planes_parsed][0],
                       &par->scat_planes[ctx->scat_planes_parsed][1],
                       &par->scat_planes[ctx->scat_planes_parsed][2],
                       &par->scat_planes[ctx->scat_planes_parsed][3]) !=
                DETSCAT_DDSCAT_SCAT_PLANE_PARAMS) {
                snprintf(parser->errmsg, sizeof(parser->errmsg),
                         "Invalid format for scattering plane at line %d",
                         parser->lineno);
                parser->status = DETSCAT_PARSER_ERR_FORMAT;
                goto error_cleanup;
            }

            ctx->scat_planes_parsed++;
            if (ctx->scat_planes_parsed == par->n_scat_planes) {
                ctx->state = PARSE_INITIAL;
            }
            return true;
    }

error_cleanup:
    detscat_ddscat_par_free_count(par, ctx->components_allocated);
    return false;
}

bool detscat_ddscat_par_load(const char *file_path,
                             DetScatDdscatParams *par,
                             DetScatDiagnose *diag) {
    assert(par != NULL);
    assert(diag != NULL);

    if (!file_path || !*file_path) {
        DETSCAT_SET_DIAGNOSE(diag, DETSCAT_ERR_INVALID_ARG, "%s",
                             "Function argument 'file_path' is invalid");
        return false;
    }

    DetScatParser parser;
    if (!detscat_parser_init(&parser, file_path)) {
        DETSCAT_SET_DIAGNOSE(diag, DETSCAT_ERR_PARSING,
                             "Could not initialize parser: %s", parser.errmsg);
        return false;
    }


    DdscatParParserContext ctx = {0};
    while (detscat_parser_next_line(&parser)) {
        if (!detscat_ddscat_parse_par_line(&parser, par, &ctx)) {
            detscat_parser_free(&parser);
            DETSCAT_SET_DIAGNOSE(diag, DETSCAT_ERR_PARSING,
                                 "Could not parse '%s': %s", file_path,
                                 parser.errmsg);
            return false;
        }
    }

    if (!parser.eof) {
        detscat_parser_free(&parser);
        detscat_parser_handle_stream_error(&parser, file_path, diag);
        return false;
    }

    if (ctx.components_allocated != par->n_components) {
        snprintf(parser.errmsg, sizeof(parser.errmsg),
                 "Expected %zu components but got %zu", par->n_components,
                 ctx.components_allocated);
        parser.status = DETSCAT_PARSER_ERR_FORMAT;
        detscat_ddscat_par_free_count(par, ctx.components_allocated);
        detscat_parser_free(&parser);
        return false;
    }
    if (ctx.scat_planes_parsed != par->n_scat_planes) {
        snprintf(parser.errmsg, sizeof(parser.errmsg),
                 "Expected %zu scattering planes but got %zu",
                 par->n_scat_planes, ctx.scat_planes_parsed);
        parser.status = DETSCAT_PARSER_ERR_FORMAT;
        detscat_ddscat_par_free_count(par, ctx.components_allocated);
        detscat_parser_free(&parser);
        return false;
    }

    detscat_parser_free(&parser);
    return true;
}

bool detscat_ddscat_fml_load(const char *file_path, DetScatDdscatFml *fml, 
                             DetScatDdscatParams *par, DetScatDiagnose *diag) {
    assert(fml != NULL);
    assert(par != NULL);
    assert(diag != NULL);

    if (!file_path || !*file_path) {
        DETSCAT_SET_DIAGNOSE(diag, DETSCAT_ERR_INVALID_ARG, "%s",
                             "Function argument 'file_path' is invalid");
        return false;
    }

    DetScatParser parser;
    if (!detscat_parser_init(&parser, file_path)) {
        DETSCAT_SET_DIAGNOSE(diag, DETSCAT_ERR_PARSING,
                             "Could not initialize parser: %s", parser.errmsg);
        return false;
    }

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

        if (!fml->fmats[i].theta || !fml->fmats[i].f11 || !fml->fmats[i].f12 ||
            !fml->fmats[i].f21 || !fml->fmats[i].f22) {
            snprintf(
                parser->errmsg, sizeof(parser->errmsg),
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
                         "Could not read line %d", parser->line_number);
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
                         "Could not parse line %d", parser->line_number);
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
            str_free(par->components[i]);
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
