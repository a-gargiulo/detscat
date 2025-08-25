#ifndef DETSCAT_PARSER_H
#define DETSCAT_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "str.h"

#define DETSCAT_PARSER_ERRMSG_MAX 256

typedef enum {
    DETSCAT_PARSER_OK = 0,
    DETSCAT_PARSER_EOF,
    DETSCAT_PARSER_ERR_INVALID_ARG,
    DETSCAT_PARSER_ERR_INVALID_STREAM,
    DETSCAT_PARSER_ERR_FILE_OPEN,
    DETSCAT_PARSER_ERR_ALLOC,
    DETSCAT_PARSER_ERR_OVERFLOW,
    DETSCAT_PARSER_ERR_FORMAT,
    DETSCAT_PARSER_ERR_UNKNOWN_KEY,
    DETSCAT_PARSER_ERR_SYNTAX, // not used
    DETSCAT_PARSER_ERR_INVALID_NUMBER, // not used
    DETSCAT_PARSER_ERR_RANGE,
    DETSCAT_PARSER_ERR_EOF, // not used
    DETSCAT_PARSER_ERR_UNKNOWN // not used
} DetScatParserStatus;

typedef struct {
    FILE                *stream;
    Str                  line;
    int                  lineno;
    DetScatParserStatus  status;
    bool                 eof;
    char                 errmsg[DETSCAT_PARSER_ERRMSG_MAX];
} DetScatParser;

bool detscat_parser_init(DetScatParser *parser, const char *file_path);

bool detscat_parser_reset(DetScatParser *parser, const char *file_path);

bool detscat_parser_next_line(DetScatParser *parser);

void detscat_parser_free(DetScatParser *parser);

#endif  // DETSCAT_PARSER_H
