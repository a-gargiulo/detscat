#ifndef DETSCAT_CONFIG_PARSER_H
#define DETSCAT_CONFIG_PARSER_H

#include <stdbool.h>
#include <stdio.h>

#include "detscat_config.h"

#define DETSCAT_CONFIG_PARSER_LINE_MAX 1024
#define DETSCAT_CONFIG_PARSER_ERR_MSG_MAX 256

typedef enum {
    DETSCAT_CONFIG_PARSER_OK = 0,
    DETSCAT_CONFIG_PARSER_ERR_FILE_NOT_FOUND,
    DETSCAT_CONFIG_PARSER_ERR_ALLOC,
    DETSCAT_CONFIG_PARSER_ERR_INVALID_ARG,
    DETSCAT_CONFIG_PARSER_ERR_FORMAT,
    DETSCAT_CONFIG_PARSER_ERR_UNKNOWN_KEY,
    DETSCAT_CONFIG_PARSER_ERR_INVALID_VALUE
} DetScatConfigParserStatus;

typedef struct {
    FILE *file;                                             // Open config file
    char line[DETSCAT_CONFIG_PARSER_LINE_MAX];              // Current line buffer
    int line_number;                                        // Current line number
    bool eof;                                               // End-of-file flag
    DetScatConfigParserStatus status;                       // Parser status code
    char error_message[DETSCAT_CONFIG_PARSER_ERR_MSG_MAX];  // Last error message
} DetScatConfigParser;

// Open and initialize parser from file path; returns parser or NULL on failure.
DetScatConfigParser *detscat_config_parser_create(const char *file_path);

// Parse file into config struct; returns true on success, false on error.
bool detscat_config_parser_parse(DetScatConfigParser *parser, DetScatConfig *config);

// Close file and free parser memory.
void detscat_config_parser_free(DetScatConfigParser *parser);

#endif  // DETSCAT_CONFIG_PARSER_H
