#ifndef DETSCAT_PARSER_H
#define DETSCAT_PARSER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "detscat_cfg.h"
#include "detscat_ddscat.h"
#include "detscat_prt.h"
#include "detscat_str.h"



/* ==========================================================================
 * Type-specific parser signatures 
 * ========================================================================== */
#define DETSCAT_CFG_MAGIC 0x43464778  // CFGx
#define DETSCAT_PRT_MAGIC 0x50525478  // PRTx
#define DETSCAT_PAR_MAGIC 0x50415278  // PARx
#define DETSCAT_FML_MAGIC 0x464D4C78  // FMLx


/* ==========================================================================
 * Parser types 
 * ========================================================================== */
#define DETSCAT_PARSER_TYPE_LIST \
    X(CFG, "CFG parser type")    \
    X(PRT, "PRT parser type")    \
    X(PAR, "PAR parser type")    \
    X(FML, "FML parser type")

typedef enum {
#define X(name, str) DETSCAT_##name,
    DETSCAT_PARSER_TYPE_LIST
#undef X
    DETSCAT_PARSER_TYPE_COUNT
} DetScatParserType;



/* ==========================================================================
 * Parser status codes 
 * ========================================================================== */
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



/* ==========================================================================
 * Type-specific parser states 
 * ========================================================================== */
typedef enum {
    PRT_STATE_INITIAL = 0,
    PRT_STATE_WAIT_SECTION,
    PRT_STATE_PARSE_TYPES_META,
    PRT_STATE_PARSE_TYPES_DEF,
    PRT_STATE_PARSE_PARTICLES_META,
    PRT_STATE_PARSE_PARTICLES_DEF,
    PRT_STATE_ERROR
} DetScatParserPrtState;

typedef enum {
    PAR_STATE_INITIAL = 0,
    PAR_STATE_PARSE_COMPONENTS,
    PAR_STATE_PARSE_SCAT_PLANES,
    PAR_STATE_PARSE_ORIENTATIONS,
    PAR_STATE_PARSE_WAVELENGTHS,
    PAR_STATE_PARSE_RADII
} DetScatParserParState;

typedef enum {
    FML_STATE_INITIAL = 0,
    FML_STATE_FIND_HEADER,
    FML_STATE_ALLOC_FMATS,
    FML_STATE_ALLOC_SCAT_PLANE,
    FML_STATE_READ_VALUES,
    FML_STATE_NEXT_SCAT_PLANE,
    FML_STATE_DONE
} DetScatParserFmlState;



/* ==========================================================================
 * Type-specific parser contexts 
 * ========================================================================== */
typedef struct {
    uint32_t magic;
    DetScatConfig *cfg;
} DetScatParserCfgContext;

typedef struct {
    uint32_t magic;
    DetScatPrt *prt;
    DetScatParserPrtState state;
    size_t types_allocated;
    size_t particles_allocated;
    bool particles_parsed;
    bool types_parsed;
} DetScatParserPrtContext;

typedef struct {
    uint32_t magic;
    DetScatDdscatParams *par;
    DetScatParserParState state;
    DetScatDdscatSamplingParams beta_params;
    DetScatDdscatSamplingParams theta_params;
    DetScatDdscatSamplingParams phi_params;
    DetScatDdscatSamplingParams radius_params;
    DetScatDdscatSamplingParams wavelength_params;
    size_t angles_parsed;
    size_t components_allocated;
    size_t scat_planes_parsed;
} DetScatParserParContext;

typedef struct {
    uint32_t magic;
    DetScatDdscatFml *fml;
    DetScatDdscatParams *par;
    DetScatParserFmlState state;
    size_t current_plane;
    size_t current_theta;
    size_t matrices_allocated;
    bool data_header_found;
} DetScatParserFmlContext;



/* ==========================================================================
 * Parser 
 * ========================================================================== */
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



/* ==========================================================================
 * API
 * ========================================================================== */

/* --------------------------------------------------------------------------
 * Lifecycle management
 * -------------------------------------------------------------------------- */
DetScatParser *detscat_parser_create(DetScatParserType type);
bool detscat_parser_init(DetScatParser *parser, const char *file_path,
                         void *ctx);
void detscat_parser_destroy(DetScatParser **parser);

/* --------------------------------------------------------------------------
 * Core operation
 * -------------------------------------------------------------------------- */
bool detscat_parser_next_line(DetScatParser *parser);
bool detscat_parser_parse_line(DetScatParser *parser);

/* --------------------------------------------------------------------------
 * Validation / finalization
 * -------------------------------------------------------------------------- */
bool detscat_parser_check_final_state_prt(DetScatParser *parser,
                                          const char *file_path, void *ctx);

bool detscat_parser_check_final_state_par(DetScatParser *parser,
                                          const char *file_path, void *ctx);

/* --------------------------------------------------------------------------
 * Utilities
 * -------------------------------------------------------------------------- */
const char *detscat_parser_type_repr(DetScatParserType type);

#define PARSER_SET_ERROR(parser, sts, fmt, ...)                            \
    do {                                                                   \
        (parser)->status = (sts);                                          \
        snprintf((parser)->error_message, sizeof((parser)->error_message), \
                 fmt, ##__VA_ARGS__);                                      \
    } while (0)

#endif  // DETSCAT_PARSER_H
