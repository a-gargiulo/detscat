#ifndef DETSCAT_PARSER_H
#define DETSCAT_PARSER_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define DETSCAT_CFG_MAGIC 0x43464778 //CFGx
#define DETSCAT_PRT_MAGIC 0x50525478 //PRTx


typedef struct DetScatParser DetScatParser;  // opaque

typedef struct DetScatConfig DetScatConfig;
typedef struct DetScatPrt DetScatPrt;

typedef struct {
    uint32_t magic;
    DetScatConfig *cfg;
} DetScatParserCfgContext;

typedef enum {
    STATE_INITIAL = 0,
    STATE_WAIT_SECTION,
    STATE_PARSE_TYPES_META,
    STATE_PARSE_TYPES_DEF,
    STATE_PARSE_PARTICLES_META,
    STATE_PARSE_PARTICLES_DEF,
    STATE_ERROR
} DetScatParserPrtState;

typedef struct {
    uint32_t magic;
    DetScatPrt *prt;
    size_t types_allocated;
    size_t particles_allocated;
    DetScatParserPrtState state;
    bool types_parsed;
    bool particles_parsed;
} DetScatParserPrtContext;

typedef enum {
    DETSCAT_CFG,
    DETSCAT_PRT,
    DETSCAT_PARSER_TYPE_COUNT
} DetScatParserType;

typedef enum {
    DETSCAT_PARSER_OK = 0,
    DETSCAT_PARSER_EOF,
    DETSCAT_PARSER_ERR_ARGUMENT,
    DETSCAT_PARSER_ERR_CONTEXT,
    DETSCAT_PARSER_ERR_FILE,
    DETSCAT_PARSER_ERR_FORMAT,
    DETSCAT_PARSER_ERR_MEMORY,
    DETSCAT_PARSER_ERR_OVERFLOW,
    DETSCAT_PARSER_ERR_RANGE,
    DETSCAT_PARSER_ERR_STREAM,
    DETSCAT_PARSER_ERR_SYNTAX,
    DETSCAT_PARSER_ERR_UNDERFLOW,
    DETSCAT_PARSER_ERR_UNKNOWN_KEY,
    DETSCAT_PARSER_ERR_UNKNOWN,
    DETSCAT_PARSER_STATUS_COUNT
} DetScatParserStatus;

DetScatParser *detscat_parser_create(DetScatParserType type);
void detscat_parser_destroy(DetScatParser **parser);
bool detscat_parser_init(DetScatParser *parser, const char *input_source, void *context); 
bool detscat_parser_next_line(DetScatParser *parser);
bool detscat_parser_parse_line(DetScatParser *parser);

const char *detscat_parser_error_message(const DetScatParser *parser);
char *detscat_parser_line_buffer(const DetScatParser *parser);
bool detscat_parser_eof(const DetScatParser *parser);

FILE *detscat_parser_stream(const DetScatParser *parser);


bool detscat_parser_reset(DetScatParser *parser, const char *file_path);


void detscat_parser_set_status(DetScatParser *parser, DetScatParserStatus status);

bool detscat_parser_set_error_message(DetScatParser *parser, const char* error_message);

DetScatParserStatus detscat_parser_status(const DetScatParser *parser);


#define PARSER_SET_ERROR(parser, sts, fmt, ...) \
    do { \
        (parser)->status = (sts); \
        snprintf((parser)->error_message, sizeof((parser)->error_message), fmt, ##__VA_ARGS__); \
    } while (0)




#endif  // DETSCAT_PARSER_H
