#include "detscat_cfg.h"

#include "detscat_diag.h"
#include "detscat_limits.h"
#include "detscat_log.h"
#include "detscat_parser.h"
#include "detscat_str.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool detscat_cfg_handle_stream_error(DetScatParser *parser,
                                            const char *file_path,
                                            DetScatDiagnose *diag); 

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------
bool detscat_cfg_create(DetScatConfig **cfg, DetScatDiagnose *diag) {
    assert(cfg != NULL);

    *cfg = calloc(1, sizeof(**cfg));
    if (!(*cfg)) goto fail;

    if (!detscat_str_init(&(*cfg)->particles_file_path) ||
        !detscat_str_reserve(&(*cfg)->particles_file_path,
                             DETSCAT_CFG_PATH_INIT)) {
        detscat_str_free(&(*cfg)->particles_file_path);
        free(*cfg);
        *cfg = NULL;
        goto fail;
    }

    return true;

fail:
    DETSCAT_SET_DIAG(diag, DETSCAT_ERR_MEMORY,
                     "Failed to allocate memory for configuration data");
    return false;
}

void detscat_cfg_destroy(DetScatConfig **cfg) {
    if (!cfg || !*cfg) return;
    detscat_str_free(&(*cfg)->particles_file_path);
    free(*cfg);
    *cfg = NULL;
}

bool detscat_cfg_load(const char *file_path, DetScatConfig *cfg,
                      DetScatDiagnose *diag) {
    assert(cfg != NULL);

    if (!file_path || !*file_path) {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_INVALID_ARGUMENT,
                         "Invalid configuration file path");
        return false;
    }

    DetScatParser *parser = detscat_parser_create(DETSCAT_CFG);
    if (!parser) {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_MEMORY,
                         "Could not create parser from '%s'", file_path);
        return false;
    }

    DetScatParserCfgContext ctx = {
        .magic = DETSCAT_CFG_MAGIC,
        .cfg = cfg,
    };

    if (!detscat_parser_init(parser, file_path, &ctx)) {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_PARSE,
                         "Could not initialize parser from '%s': %s", file_path,
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
        detscat_cfg_handle_stream_error(parser, file_path, diag);
        detscat_parser_destroy(&parser);
        return false;
    }

    detscat_parser_destroy(&parser);
    detscat_log_info("Successfully parsed '%s'", file_path);
    return true;
}

//------------------------------------------------------------------------------
// Interal Helpers 
//------------------------------------------------------------------------------
static bool detscat_cfg_handle_stream_error(DetScatParser *parser,
                                            const char *file_path,
                                            DetScatDiagnose *diag) {
    assert(parser != NULL);

    if (!file_path || !*file_path) {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_INVALID_ARGUMENT,
                         "Invalid configuration file path");
        return false;
    }

    const char *parser_error_message = detscat_parser_error_message(parser);
    if (parser_error_message[0] != '\0') {
        DETSCAT_SET_DIAG(diag, DETSCAT_ERR_PARSE, "Could not parse '%s': %s",
                         file_path, parser_error_message);
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
