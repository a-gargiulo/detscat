#ifndef DETSCAT_H
#define DETSCAT_H

#include <stdbool.h>

/* ==========================================================================
 * Main DetScat context
 * ========================================================================== */
typedef struct DetScat DetScat;

/* Error tracking */
typedef struct DetScatError DetScatError;

/* ==========================================================================
 * DetScat program status codes
 * ========================================================================== */
typedef enum {
    DETSCAT_OK,
    DETSCAT_ERR_CMD_ARG,
    DETSCAT_ERR_IMAGE,
    DETSCAT_ERR_INVALID_ARG,
    DETSCAT_ERR_KEY_LOOKUP,
    DETSCAT_ERR_MEMORY,
    DETSCAT_ERR_MSG_ENCODE,
    DETSCAT_ERR_OVERFLOW,
    DETSCAT_ERR_PARSE,
    DETSCAT_ERR_RANGE,
    DETSCAT_ERR_UNDERFLOW,
    DETSCAT_ERR_UNKNOWN,
    DETSCAT_STATUS_COUNT
} DetScatStatus;

/* ==========================================================================
 * Logging levels
 * ========================================================================== */
typedef enum {
    DETSCAT_DEBUG,
    DETSCAT_INFO,
    DETSCAT_WARNING,
    DETSCAT_ERROR
} DetScatLogLevel;

/* ==========================================================================
 * Core functions
 * ========================================================================== */

/* Detscat context lifecycle */
DetScat *detscat_create(const char *cfg_file_path, DetScatError *err);
void detscat_destroy(DetScat **detscat);

/* Load simulation data */
bool detscat_load_data(DetScat *detscat, DetScatError *err);

bool detscat_setup_camera(DetScat *detscat, DetScatError *err);

void detscat_simulation_run(DetScat *detscat);

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
DetScatError *detscat_error_create(void);
void detscat_error_destroy(DetScatError **err);
DetScatStatus detscat_error_status(const DetScatError *err);
const char *detscat_error_message(const DetScatError *err);

/* ==========================================================================
 * Logging
 * ========================================================================== */
void detscat_log(DetScatLogLevel level, const char *fmt, ...);
void detscat_log_error(const DetScatError *err);

#endif  //  DETSCAT_H
