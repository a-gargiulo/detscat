#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "config_parser.h"


ConfigParser *detscat_config_parser_init(const char *config_file_path) {

    ConfigParser *parser = malloc(sizeof(ConfigParser));
    if (!parser) return NULL;
    
    parser->file = fopen(config_file_path, "r");
    if (!parser->file) {
        free(parser);
        return NULL;
    }

    parser->line_number = 0;
    parser->eof = false;
    parser->error_code = CONFIG_PARSER_OK;
    parser->line[0] = '\0';  

    return parser;
}
