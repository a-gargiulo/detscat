#ifndef DETSCAT_PARSER_H
#define DETSCAT_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "str.h"

#define DETSCAT_PARSER_ERRMSG_MAX 256
#define DETSCAT_PARSER_LINE_INIT 1024
#define DETSCAT_PARSER_LINE_MAX 65536  // 64kB

typedef enum {
    DETSCAT_PARSER_OK = 0,
    DETSCAT_PARSER_EOF,
    DETSCAT_PARSER_ERR_INVALID_ARG,
    DETSCAT_PARSER_ERR_INVALID_STREAM,
    DETSCAT_PARSER_ERR_FILE_OPEN,
    DETSCAT_PARSER_ERR_ALLOC,
    DETSCAT_PARSER_ERR_OVERFLOW,
    DETSCAT_PARSER_ERR_EOF,

    DETSCAT_PARSER_ERR_FORMAT,
    DETSCAT_PARSER_ERR_SYNTAX,
    DETSCAT_PARSER_ERR_INVALID_NUMBER,
    DETSCAT_PARSER_ERR_UNKNOWN
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
