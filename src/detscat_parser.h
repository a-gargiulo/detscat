#ifndef DETSCAT_PARSER_H
#define DETSCAT_PARSER_H

#include "detscat_cfg.h"
#include "detscat_prt.h"
#include "detscat_ddscat.h"
#include "detscat_str.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


#define DETSCAT_CFG_MAGIC 0x43464778  // CFGx
#define DETSCAT_PRT_MAGIC 0x50525478  // PRTx
#define DETSCAT_PAR_MAGIC 0x50415278  // PARx


#define DETSCAT_PARSER_TYPE_LIST        \
    X(CFG, "CFG parser type")           \
    X(PRT, "PRT parser type")           \
    X(PAR, "PAR parser type")

typedef enum {
#define X(name, str) DETSCAT_##name,
    DETSCAT_PARSER_TYPE_LIST
#undef X
        DETSCAT_PARSER_TYPE_COUNT
} DetScatParserType;

typedef enum {
    DETSCAT_PARSER_OK = 0,
    DETSCAT_PARSER_EOF,
    DETSCAT_PARSER_ERR_INVALID_ARG,
    DETSCAT_PARSER_ERR_CONTEXT,
    DETSCAT_PARSER_ERR_FILE,
    DETSCAT_PARSER_ERR_FORMAT,
    DETSCAT_PARSER_ERR_MEMORY,
    DETSCAT_PARSER_ERR_OVERFLOW,
    DETSCAT_PARSER_ERR_RANGE,
    DETSCAT_PARSER_ERR_STREAM,
    DETSCAT_PARSER_ERR_SYNTAX,
    DETSCAT_PARSER_ERR_TYPE,
    DETSCAT_PARSER_ERR_UNDERFLOW,
    DETSCAT_PARSER_ERR_UNKNOWN_KEY,
    DETSCAT_PARSER_ERR_UNKNOWN,
    DETSCAT_PARSER_STATUS_COUNT
} DetScatParserStatus;

typedef struct {
    FILE *stream;
    void *context;
    Str current_line;
    int line_number;
    DetScatParserStatus status;
    DetScatParserType type;
    bool eof;
    char error_message[256];
} DetScatParser;

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
    PAR_INITIAL = 0,
    PAR_COMP,
    PAR_PLANES
} DetScatParserParState;

typedef struct {
   uint32_t magic; 
   DetScatDdscatParams *par;
   DetScatParserParState state;
} DetScatParserParContext;


DetScatParser *detscat_parser_create(DetScatParserType type);
void detscat_parser_destroy(DetScatParser **parser);

bool detscat_parser_init(DetScatParser *parser, const char *file_path,
                         void *context);
bool detscat_parser_reset(DetScatParser *parser, const char *file_path,
                          void *context);

bool detscat_parser_next_line(DetScatParser *parser);
bool detscat_parser_parse_line(DetScatParser *parser);

const char *detscat_parser_type_repr(DetScatParserType type);

bool detscat_parser_check_final_state_prt(DetScatParser *parser,
                                          const char *file_path,
                                          void *context);

#define PARSER_SET_ERROR(parser, sts, fmt, ...)                            \
    do {                                                                   \
        (parser)->status = (sts);                                          \
        snprintf((parser)->error_message, sizeof((parser)->error_message), \
                 fmt, ##__VA_ARGS__);                                      \
    } while (0)

#endif  // DETSCAT_PARSER_H
