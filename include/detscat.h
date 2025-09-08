#ifndef DETSCAT_H
#define DETSCAT_H

#include <stdbool.h>

/* Main DetScat context */
typedef struct DetScat DetScat;

/* Error tracking */
typedef struct DetScatError DetScatError;

/* DetScat program status*/
typedef enum {
    DETSCAT_OK,
    DETSCAT_ERR_CMD_ARG,
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

/* Log levels */
typedef enum {
    DETSCAT_DEBUG,
    DETSCAT_INFO,
    DETSCAT_WARNING,
    DETSCAT_ERROR
} DetScatLogLevel;

// --- DetScat ---
DetScat *detscat_create(const char* cfg_file_path, DetScatError *err);
void detscat_destroy(DetScat **detscat);
bool detscat_load_data(DetScat* detscat, DetScatError* err);

void detscat_print_cfg(const DetScat *detscat);

// --- System ---
bool detscat_init(DetScatError *err);
void detscat_shutdown(void);

// --- Error tracking ---
DetScatError *detscat_error_create(void);
void detscat_error_destroy(DetScatError **err);
DetScatStatus detscat_error_status(const DetScatError *err);
const char *detscat_error_message(const DetScatError *err);

// --- Logging ---
void detscat_log(DetScatLogLevel level, const char *fmt, ...);
void detscat_log_error(const DetScatError *err);



// void detscat_data_free(DetScat *detscat);


#endif  //  DETSCAT_H
