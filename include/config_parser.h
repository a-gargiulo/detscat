#ifndef DETSCAT_CONFIG_PARSER_H
#define DETSCAT_CONFIG_PARSER_H

#include <stdio.h>
#include <stdbool.h>

#include "config.h"

#define MAX_LINE_LENGTH 1024
#define MAX_ERROR_MESSAGE_LENGTH 256

typedef enum {
    CONFIG_PARSER_OK = 0,
    CONFIG_PARSER_ERR_FILE_NOT_FOUND,
    CONFIG_PARSER_ERR_ALLOCATION_FAILED,
    CONFIG_PARSER_ERR_INVALID_ARGUMENT,
    CONFIG_PARSER_ERR_INVALID_FORMAT,
    CONFIG_PARSER_ERR_UNKNOWN_KEY,
    CONFIG_PARSER_ERR_INVALID_VALUE
} ConfigParserError;

typedef struct {
    FILE *file;                  
    char line[MAX_LINE_LENGTH];  
    int line_number;             
    bool eof;                    
    ConfigParserError error_code;              
    char error_message[MAX_ERROR_MESSAGE_LENGTH];
} ConfigParser;


ConfigParser *detscat_config_parser_init(const char *config_file_path);
bool detscat_config_parser_parse(ConfigParser *parser, Config *config);
void detscat_config_parser_free(ConfigParser *parser);




#endif  // DETSCAT_CONFIG_PARSER_H
