#include "detscat_prt.h"

#include "detscat_diag.h"
#include "detscat_str.h"
#include "detscat_parser.h"
#include "detscat_log.h"
// #include "detscat_limits.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <strings.h>
#include <errno.h>
// #include <limits.h>
// #include <math.h>

// #include "detscat_log.h"
// #include "detscat_parser.h"
// #include "detscat_ddscat.h"
// #include "str.h"

bool detscat_prt_create(DetScatPrt **prt, DetScatDiagnose *diag) {
    assert(prt != NULL);

    *prt = calloc(1, sizeof(**prt));
    if (!(*prt)) {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_MEMORY,
                "Failed to allocate memory for particle data");
        return false;
    }

    return true;
}

static void detscat_prt_clear_types(DetScatPrt *prt, size_t count) {
    if (!prt || !prt->types) return;

    for (size_t i = 0; i < count; ++i) {
        detscat_str_free(&prt->types[i].type_id);
        detscat_str_free(&prt->types[i].data_dir);
    }

    free(prt->types);
    prt->types = NULL;
}

static void detscat_prt_clear_particles(DetScatPrt *prt, size_t count) {
    if (!prt || !prt->particles) return;

    for (size_t i = 0; i < count; ++i) {
        detscat_str_free(&prt->particles[i].type_id);
        free(prt->particles[i].case_id);
        prt->particles[i].case_id = NULL;

        memset(&prt->particles[i].position, 0,
               sizeof(prt->particles[i].position));
    }

    free(prt->particles);
    prt->particles = NULL;
}


void detscat_prt_destroy(DetScatPrt **prt) {
    if (!prt) return;

    detscat_prt_clear_types(*prt, (*prt)->n_types);
    detscat_prt_clear_particles(*prt, (*prt)->n_particles);

    (*prt)->n_types = 0;
    (*prt)->n_particles = 0;

    free(*prt); 
    *prt = NULL;
}


void detscat_prt_free_subset(DetScatPrt *prt, size_t types_count,
                                   size_t particles_count) {
    if (!prt) return;

    detscat_prt_clear_types(prt, types_count);
    detscat_prt_clear_particles(prt, particles_count);

    prt->n_types = 0;
    prt->n_particles = 0;
}


static bool detscat_prt_check_final_state(DetScatParser *parser,
                                          DetScatPrt *prt,
                                          DetScatParserPrtContext *ctx,
                                          const char *file_path,
                                          DetScatDiagnose *diag) {
    assert(parser != NULL);
    assert(prt != NULL);
    assert(ctx != NULL);

    if (!file_path || !*file_path) {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_INVALID_ARGUMENT,
                         "Function argument 'file_path' is invalid");
        return false;
    }

    switch (ctx->state) {
        case STATE_PARSE_TYPES_DEF:
            detscat_parser_set_status(parser, DETSCAT_PARSER_ERR_FORMAT);
            detscat_parser_set_error_message(
                parser, 
                "Unexpected end of file: missing $(EndTypes)");
            break;

        case STATE_PARSE_PARTICLES_DEF:
            detscat_parser_set_status(parser, DETSCAT_PARSER_ERR_FORMAT);
            detscat_parser_set_error_message(
                parser, 
                "Unexpected end of file: missing $(EndParticles)");
            break;

        default:
            if (!ctx->types_parsed || !ctx->particles_parsed) {
                detscat_parser_set_status(parser, DETSCAT_PARSER_ERR_FORMAT);

                if (!ctx->types_parsed && !ctx->particles_parsed) {
                    detscat_parser_set_error_message(
                        parser, 
                        "Missing sections: "
                        "$(StartTypes) and $(StartParticles)");
                } else if (!ctx->types_parsed) {
                    detscat_parser_set_error_message(
                        parser, 
                        "Missing section: $(StartTypes)");
                } else {
                    detscat_parser_set_error_message(
                        parser, 
                        "Missing section: $(StartParticles)");
                }
                break;
            }
            return true;
    }

    detscat_prt_free_subset(prt, ctx->types_allocated, ctx->particles_allocated);

    DETSCAT_SET_DIAG(diag, DETSCAT_ERR_PARSE, "Could not parse '%s': %s",
                         file_path, detscat_parser_error_message(parser));
    return false;
}
//
//
//
//
//
//
//
//
//
//
//




static bool detscat_prt_handle_stream_error(DetScatParser *parser,
                                            const char *file_path,
                                            DetScatDiagnose *diag) {
    assert(parser != NULL);

    if (!file_path || !*file_path) {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_INVALID_ARGUMENT,
                         "Function argument 'file_path' is invalid");
        return false;
    }

    const char *parser_errmsg = detscat_parser_error_message(parser);
    if (parser_errmsg[0] != '\0') {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_PARSE,
                         "Could not parse '%s': %s", file_path,
                         parser_errmsg);
    } else if (ferror(detscat_parser_stream(parser))) {
        int err = errno;
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_PARSE,
                         "I/O error while reading '%s': %s", file_path,
                             strerror(err));
    } else {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_PARSE,
                         "Could not parse '%s' (unknown error)", file_path);
    }

    return false;
}












bool detscat_prt_load(const char *file_path, DetScatPrt *prt,
                      DetScatDiagnose *diag) {
    assert(prt != NULL);

    if (!file_path || !*file_path) {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_INVALID_ARGUMENT,
                         "Invalid particles file path");
        return false;
    }


    DetScatParser *parser = detscat_parser_create(DETSCAT_PRT);
    if (!parser) {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_MEMORY,
                         "Could not create particles file parser");
        return false;
    }

    DetScatParserPrtContext ctx = {
        .magic = DETSCAT_PRT_MAGIC,
        .prt = prt,
        .types_allocated = 0,
        .particles_allocated = 0,
        .state = STATE_INITIAL,
        .types_parsed = false,
        .particles_parsed = false
    };

    if (!detscat_parser_init(parser, file_path, &ctx)) {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_PARSE,
                         "Could not initialize particles file parser: %s",
                         detscat_parser_error_message(parser));
        detscat_parser_destroy(&parser);
        return false;
    }

    while (detscat_parser_next_line(parser)) {
        if (!detscat_parser_parse_line(parser)) {
            DETSCAT_SET_DIAG(diag, DETSCAT_ERR_PARSE,
                             "Could not parse '%s': %s", file_path,
                             detscat_parser_error_message(parser));
            detscat_parser_destroy(&parser);
            return false;
        }
    }

    if (!detscat_parser_eof(parser)) {
        detscat_prt_handle_stream_error(parser, file_path, diag);
        detscat_parser_destroy(&parser);
        return false;
    }

    if (!detscat_prt_check_final_state(parser, prt, &ctx, file_path, diag)) {
        detscat_parser_destroy(&parser);
        return false;
    }

    detscat_parser_destroy(&parser);

    detscat_log_info("Successfully parsed '%s'", file_path);
    return true;
}

