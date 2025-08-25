#include "detscat_parser.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

bool detscat_parser_init(DetScatParser *parser, const char *file_path) {
    assert(parser != NULL);

    memset(parser, 0, sizeof(DetScatParser));

    if (!file_path || file_path[0] == '\0') {
        parser->status = DETSCAT_PARSER_ERR_INVALID_ARG;
        snprintf(parser->errmsg, sizeof(parser->errmsg), "Invalid file path.");
        return false;
    }

    errno = 0;
    parser->stream = fopen(file_path, "r");
    if (!parser->stream) {
        parser->status = DETSCAT_PARSER_ERR_FILE_OPEN;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Could not open '%s': %s", file_path, strerror(errno));
        return false;
    }

    if (!str_init(&parser->line)) {
        fclose(parser->stream);
        parser->stream = NULL;
        parser->status = DETSCAT_PARSER_ERR_ALLOC;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Memory allocation for line buffer failed");
        return false;
    }

    if (!str_reserve(&parser->line, DETSCAT_PARSER_LINE_INIT)) {
        str_free(&parser->line);
        fclose(parser->stream);
        parser->stream = NULL;
        parser->status = DETSCAT_PARSER_ERR_ALLOC;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Memory allocation (reserve) for line buffer failed");
        return false;
    }

    parser->lineno = 0;
    parser->eof = false;
    parser->errmsg[0] = '\0';
    parser->status = DETSCAT_PARSER_OK;

    return true;
}

bool detscat_parser_reset(DetScatParser *parser, const char *file_path) {
    assert(parser != NULL);

    detscat_parser_free(parser);

    return detscat_parser_init(parser, file_path);
}

bool detscat_parser_next_line(DetScatParser *parser) {
    assert(parser != NULL);

    if (!parser->stream) {
        parser->status = DETSCAT_PARSER_ERR_INVALID_STREAM;
        snprintf(parser->errmsg, sizeof(parser->errmsg), "Stream is not open");
        return false;
    }

    if (!parser->line.data) {
        parser->status = DETSCAT_PARSER_ERR_ALLOC;
        snprintf(parser->errmsg, sizeof(parser->errmsg),
                 "Line buffer is uninitialized");
        return false;
    }

    if (parser->eof) {
        parser->status = DETSCAT_PARSER_EOF;
        return false;
    }

    parser->line.length = 0;
    parser->line.data[0] = '\0';

    int ch;
    while ((ch = fgetc(parser->stream)) != EOF) {
        if (parser->line.length >= DETSCAT_PARSER_LINE_MAX) {
            parser->status = DETSCAT_PARSER_ERR_OVERFLOW;
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Line exceeds maximum allowed length (%d bytes)",
                     DETSCAT_PARSER_LINE_MAX);
            return false;
        }

        if (!str_append_char(&parser->line, (char)ch)) {
            parser->status = DETSCAT_PARSER_ERR_ALLOC;
            snprintf(parser->errmsg, sizeof(parser->errmsg),
                     "Failed to grow line buffer");
            return false;
        }

        if (ch == '\n') break;
    }

    if (ch == EOF) {
        if (parser->line.length == 0) {
            parser->eof = true;
            parser->status = DETSCAT_PARSER_EOF;
            return false;
        }
        parser->eof = true;
    }

    parser->lineno++;
    parser->status = DETSCAT_PARSER_OK;
    return true;
}

void detscat_parser_free(DetScatParser *parser) {
    if (!parser) return;

    if (parser->stream) {
        fclose(parser->stream);
        parser->stream = NULL;
    }

    str_free(&parser->line);

    parser->lineno = 0;
    parser->eof = false;
    parser->status = DETSCAT_PARSER_OK;
    parser->errmsg[0] = '\0';
}
