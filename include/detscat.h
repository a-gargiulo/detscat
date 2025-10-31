#ifndef DETSCAT_H
#define DETSCAT_H


#include <stdbool.h>


/* ==========================================================================
 * Enums
 * ========================================================================== */
/* Log levels */
#define DS_LOG_LEVEL_LIST                                                      \
    X(DEBUG)                                                                   \
    X(INFO)                                                                    \
    X(WARNING)                                                                 \
    X(ERROR)

typedef enum {
#define X(name) DS_##name,
    DS_LOG_LEVEL_LIST 
#undef X
} DetScatLogLevel;

/* Status codes */
#define DS_STATUS_LIST                                                         \
    X(OK, "OK")                                                                \
    X(ERR_CMD_ARG, "Invalid or missing command-line argument")                 \
    X(ERR_IMAGE, "Image generation error")                                     \
    X(ERR_INVALID_ARG, "Invalid argument")                                     \
    X(ERR_KEY_LOOKUP, "Key lookup error")                                      \
    X(ERR_MEMORY, "Memory allocation error")                                   \
    X(ERR_MSG_ENCODE, "Message encoding error")                                \
    X(ERR_OVERFLOW, "Overflow error")                                          \
    X(ERR_PARSE, "Parsing error")                                              \
    X(ERR_RANGE, "Argument out of range")                                      \
    X(ERR_UNDERFLOW, "Underflow error")                                        \
    X(UNKNOWN, "Unknown status")

typedef enum {
#define X(name, msg) DS_##name,
    DS_STATUS_LIST
#undef X
} DetScatStatus;


/* ==========================================================================
 * Structs (opaque)
 * ========================================================================== */
typedef struct DetScat DetScat;
typedef struct DetScatError DetScatError;


/* ==========================================================================
 * Core
 * ========================================================================== */
/* Detscat context lifecycle */
DetScat *detscat_create(const char *cfg_file_path, DetScatError *err);
void detscat_destroy(DetScat **detscat);

/* Load simulation data */
bool detscat_load_data(DetScat *detscat, DetScatError *err);

bool detscat_setup_camera(DetScat *detscat, DetScatError *err);

bool detscat_simulation_run(DetScat *detscat, DetScatError *err);

bool detscat_construct_image(DetScat *detscat, DetScatError *err);

/* Inspect data */
void detscat_print_cfg(const DetScat *detscat);
void detscat_print_prt(const DetScat *detscat);
void detscat_print_ddscat(const DetScat *detscat);


/* ==========================================================================
 * System level functions
 * ========================================================================== */
bool detscat_init(DetScatError *err);
void detscat_shutdown(void);


/* ==========================================================================
 * Error tracking
 * ========================================================================== */
/* Lifetime */
DetScatError *ds_error_create(void);
void ds_error_destroy(DetScatError **err);

/* Getters */
DetScatStatus ds_error_status(const DetScatError *err);
const char *ds_error_message(const DetScatError *err);


/* ==========================================================================
 * Logging
 * ========================================================================== */
void ds_log(DetScatLogLevel level, const char *fmt, ...);
void ds_log_error(const DetScatError *err);


#endif  //  DETSCAT_H
