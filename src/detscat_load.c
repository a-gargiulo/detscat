#include "detscat.h"

#include "detscat_cfg.h"
#include "detscat_ddscat.h"
#include "detscat_error.h"
#include "detscat_parser.h"
#include "detscat_prt.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// --- Internal helpers (PRIVATE)
static bool detscat_handle_stream_error(DetScatParser *parser,
                                        const char *file_path,
                                        DetScatError *err) {
    assert(parser);

    assert(parser->type >= 0 && parser->type < DETSCAT_PARSER_TYPE_COUNT);
    const char *parser_type;
    switch (parser->type) {
        case DETSCAT_CFG:
            parser_type = "cfg";
            break;

        case DETSCAT_PRT:
            parser_type = "prt";
            break;

        case DETSCAT_PAR:
            parser_type = "par";
            break;

        case DETSCAT_FML:
            parser_type = "fml";
            break;
    }

    if (!file_path || !*file_path) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_INVALID_ARG, "Invalid %s file path",
                          parser_type);
        return false;
    }

    const char *parser_error_message = parser->error_message;
    if (parser_error_message[0] != '\0') {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE, "Could not parse '%s': %s",
                          file_path, parser_error_message);
    } else if (ferror(parser->stream)) {
        int errc = errno;
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                          "I/O error while reading '%s': %s", file_path,
                          strerror(errc));
    } else {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                          "Could not parse '%s' (unknown error)", file_path);
    }

    return false;
}

static bool detscat_extract_ddscat_cases_from_par(DetScatDdscatParams *par, DetScatError *err) {
    par->n_cases = par->n_wavelengths * par->n_radii * par->n_orientations;
    par->cases = calloc(par->n_cases, sizeof(*par->cases));
    if (!par->cases) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Could not allocate memory for cases");
        return false;
    }

    size_t c = 0;
    for (size_t i = 0; i < par->n_wavelengths; ++i) {
        for (size_t j = 0; j < par->n_radii; ++j) {
            for (size_t k = 0; k < par->n_orientations; ++k) {
                par->cases[c].w = i;
                par->cases[c].r = j;
                par->cases[c].k = k;
                par->cases[c].wavelength = par->wavelengths[i]; 
                par->cases[c].radius = par->radii[j];
                par->cases[c].orientation.beta = par->orientations[k].beta;
                par->cases[c].orientation.theta = par->orientations[k].theta;
                par->cases[c].orientation.phi = par->orientations[k].phi;
                detscat_math_vec3_angles_to_orientation(
                    par->cases[c].orientation.theta,
                    par->cases[c].orientation.beta,
                    par->cases[c].orientation.phi,
                    &par->cases[c].orientation.a1,
                    &par->cases[c].orientation.a2);
                
                c++;
            }
        }
    }

    return true;
}

// --- Internal helpers (SHARED) ---
bool detscat_cfg_load(const char *cfg_file_path, DetScatConfig *cfg,
                      DetScatError *err) {
    assert(cfg);

    if (!cfg_file_path || !*cfg_file_path) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_INVALID_ARG,
                          "Invalid cfg file path");
        return false;
    }

    DetScatParser *parser = detscat_parser_create(DETSCAT_CFG);
    if (!parser) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Could not create parser of %s from '%s'",
                          detscat_parser_type_repr(DETSCAT_CFG), cfg_file_path);
        return false;
    }

    DetScatParserCfgContext ctx = {
        .magic = DETSCAT_CFG_MAGIC,
        .cfg = cfg,
    };

    if (!detscat_parser_init(parser, cfg_file_path, &ctx)) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                          "Could not initialize parser of %s from '%s': %s",
                          detscat_parser_type_repr(DETSCAT_CFG), cfg_file_path,
                          parser->error_message);
        detscat_parser_destroy(&parser);
        return false;
    }

    while (detscat_parser_next_line(parser)) {
        if (!detscat_parser_parse_line(parser)) {
            DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                              "Could not parse '%s': %s", cfg_file_path,
                              parser->error_message);
            detscat_parser_destroy(&parser);
            return false;
        }
    }

    if (!parser->eof) {
        detscat_handle_stream_error(parser, cfg_file_path, err);
        detscat_parser_destroy(&parser);
        return false;
    }

    detscat_parser_destroy(&parser);
    detscat_log(DETSCAT_INFO, "Successfully parsed '%s'", cfg_file_path);
    return true;
}

bool detscat_prt_load(const char *prt_file_path, DetScatPrt *prt, DetScatError *err) {
    assert(prt);

    if (!prt_file_path || !*prt_file_path) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_INVALID_ARG,
                          "Invalid particles file path");
        return false;
    }

    DetScatParser *parser = detscat_parser_create(DETSCAT_PRT);
    if (!parser) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Could not create particles file parser");
        return false;
    }

    DetScatParserPrtContext ctx = {.magic = DETSCAT_PRT_MAGIC,
                                   .prt = prt,
                                   .types_allocated = 0,
                                   .particles_allocated = 0,
                                   .state = PRT_STATE_INITIAL,
                                   .types_parsed = false,
                                   .particles_parsed = false};

    if (!detscat_parser_init(parser, prt_file_path, &ctx)) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                          "Could not initialize particles file parser: %s",
                          parser->error_message);
        detscat_parser_destroy(&parser);
        return false;
    }

    while (detscat_parser_next_line(parser)) {
        if (!detscat_parser_parse_line(parser)) {
            DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                              "Could not parse '%s': %s", prt_file_path,
                              parser->error_message);
            detscat_parser_destroy(&parser);
            return false;
        }
    }

    if (!parser->eof) {
        detscat_handle_stream_error(parser, prt_file_path, err);
        detscat_parser_destroy(&parser);
        return false;
    }

    if (!detscat_parser_check_final_state_prt(parser, prt_file_path, &ctx)) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE, "Could not parse '%s': %s",
                          prt_file_path, parser->error_message);
        detscat_parser_destroy(&parser);
        return false;
    }

    detscat_parser_destroy(&parser);

    detscat_log(DETSCAT_INFO, "Successfully loaded '%s'", prt_file_path);
    return true;
}

bool detscat_ddscat_par_load(const char *par_file_path,
                             DetScatDdscatParams *par, DetScatError *err) {
    assert(par);

    if (!par_file_path || !*par_file_path) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_INVALID_ARG, "%s",
                          "Invalid .par file path");
        return false;
    }

    DetScatParser *parser = detscat_parser_create(DETSCAT_PAR);
    if (!parser) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Could not create par file parser");
        return false; }

    DetScatParserParContext ctx = {
        .magic = DETSCAT_PAR_MAGIC,
        .par = par,
        .beta_params = (DetScatDdscatSamplingParams){0},
        .theta_params = (DetScatDdscatSamplingParams){0},
        .phi_params = (DetScatDdscatSamplingParams){0},
        .wavelength_params = (DetScatDdscatSamplingParams){0},
        .radius_params = (DetScatDdscatSamplingParams){0},
        .components_allocated = 0,
        .angles_parsed = 0,
        .state = PAR_STATE_INITIAL,
        .scat_planes_parsed = 0,
    };

    if (!detscat_parser_init(parser, par_file_path, &ctx)) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                          "Could not initialize par file parser: %s",
                          parser->error_message);
        detscat_parser_destroy(&parser);
        return false;
    }

    while (detscat_parser_next_line(parser)) {
        if (!detscat_parser_parse_line(parser)) {
            DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                              "Could not parse '%s': %s", par_file_path,
                              parser->error_message);
            detscat_parser_destroy(&parser);
            return false;
        }
    }

    if (!parser->eof) {
        detscat_handle_stream_error(parser, par_file_path, err);
        detscat_parser_destroy(&parser);
        return false;
    }

    if (!detscat_parser_check_final_state_par(parser, par_file_path, &ctx)) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE, "Could not parse '%s': %s",
                          par_file_path, parser->error_message);
        detscat_parser_destroy(&parser);
        return false;
    }

    detscat_parser_destroy(&parser);


    if (!detscat_extract_ddscat_cases_from_par(par, err)) return false;

    return true;
}

bool detscat_ddscat_fml_load(const char *fml_file_path, DetScatDdscatFml *fml,
                             DetScatDdscatParams *par, DetScatError *err) {
    assert(fml);

    if (!fml_file_path || !*fml_file_path) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_INVALID_ARG, "%s",
                          "Invalid .fml file path");
        return false;
    }

    DetScatParser *parser = detscat_parser_create(DETSCAT_FML);
    if (!parser) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Could not create fml file parser");
        return false;
    }

    DetScatParserFmlContext ctx = {.magic = DETSCAT_FML_MAGIC,
                                   .fml = fml,
                                   .par = par,
                                   .matrices_allocated = 0,
                                   .current_plane = 0,
                                   .current_theta = 0,
                                   .data_header_found = false,
                                   .state = FML_STATE_INITIAL};

    if (!detscat_parser_init(parser, fml_file_path, &ctx)) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                          "Could not initialize fml file parser: %s",
                          parser->error_message);
        detscat_parser_destroy(&parser);
        return false;
    }

    while (detscat_parser_next_line(parser)) {
        if (!detscat_parser_parse_line(parser)) {
            DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                              "Could not parse '%s': %s", fml_file_path,
                              parser->error_message);
            detscat_parser_destroy(&parser);
            return false;
        }
    }

    if (!parser->eof) {
        detscat_handle_stream_error(parser, fml_file_path, err);
        detscat_parser_destroy(&parser);
        return false;
    } else {
        if (!ctx.data_header_found) {
            DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                              "Reached EOF. Did not find any data header.");
            detscat_parser_destroy(&parser);
            return false;
        }
    }

    detscat_parser_destroy(&parser);

    return true;
}
