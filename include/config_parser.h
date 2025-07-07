#ifndef DETSCAT_CONFIG_PARSER_H
#define DETSCAT_CONFIG_PARSER_H

#include <stdio.h>
#include <stdbool.h>
#include "mymath.h"


#define MAX_LINE_LENGTH 1024
#define MAX_PATH_LENGTH 256
#define MAX_KEY_LENGTH  64

typedef enum {
    CONFIG_PARSER_OK = 0,
    CONFIG_PARSER_ERR_FILE_NOT_FOUND
} ConfigParserError;

typedef struct {
    FILE *file;                  
    char line[MAX_LINE_LENGTH];  
    int line_number;             
    bool eof;                    
    ConfigParserError error_code;              
} ConfigParser;

typedef enum {
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_COMPLEX_VEC3,
    TYPE_VEC3
} ValueType;

typedef struct {
    char key[MAX_KEY_LENGTH];
    ValueType type;
    union {
        int i;
        double d;
        float f;
        char *s;
        ComplexVec3 cv3;
        Vec3 v3;
    } val;
} KeyVal;

ConfigParser *detscat_config_parser_init(const char *config_file_path);



#endif  // DETSCAT_CONFIG_PARSER_H
