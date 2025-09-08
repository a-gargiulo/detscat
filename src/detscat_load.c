#include "detscat.h"

#include "detscat_cfg.h"
#include "detscat_error.h"
#include "detscat_parser.h"
#include "detscat_prt.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


// --- Internal helpers (PRIVATE)
static bool detscat_handle_stream_error(DetScatParser *parser,
                                        const char *file_path,
                                        DetScatError *err) {
    assert(parser);

    assert(parser->type >= 0 && parser->type < DETSCAT_PARSER_TYPE_COUNT);
    const char *parser_type;
    switch(parser->type) {
        case DETSCAT_CFG:
            parser_type = "cfg";
            break;
        case DETSCAT_PRT:
            parser_type = "prt";
            break;
    }

    if (!file_path || !*file_path) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_INVALID_ARG,
                          "Invalid %s file path",
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
                          "Could not parse '%s' (unknown error)",
                          file_path);
    }

    return false;
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

bool detscat_prt_load(const char *file_path, DetScatPrt *prt,
                      DetScatError *err) {
    assert(prt);

    if (!file_path || !*file_path) {
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
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                          "Could not initialize particles file parser: %s",
                          parser->error_message);
        detscat_parser_destroy(&parser);
        return false;
    }

    while (detscat_parser_next_line(parser)) {
        if (!detscat_parser_parse_line(parser)) {
            DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE,
                             "Could not parse '%s': %s", file_path,
                             parser->error_message);
            detscat_parser_destroy(&parser);
            return false;
        }
    }

    if (!parser->eof) {
        detscat_handle_stream_error(parser, file_path, err);
        detscat_parser_destroy(&parser);
        return false;
    }

    if (!detscat_parser_check_final_state_prt(parser, file_path, &ctx)) {
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_PARSE, "Could not parse '%s': %s",
                         file_path, parser->error_message);
        detscat_parser_destroy(&parser);
        return false;
    }

    detscat_parser_destroy(&parser);

    detscat_log(DETSCAT_INFO, "Successfully parsed '%s'", file_path);
    return true;
}
