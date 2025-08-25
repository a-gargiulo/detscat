#ifndef DETSCAT_DIAG_H
#define DETSCAT_DIAG_H

#include <stdio.h>

#define DETSCAT_DIAG_MSG_MAX 256

typedef enum {
    DETSCAT_OK = 0,
    DETSCAT_ERR_MISSING_CMD_ARG,
    DETSCAT_ERR_PARSING,
    DETSCAT_ERR_ALLOC,
    DETSCAT_ERR_LOOKUP,
    DETSCAT_ERR_OVERFLOW,
    DETSCAT_ERR_INVALID_ARG,
    DETSCAT_ERR_ARG_RANGE
} DetScatStatus;

typedef struct {
    const char *source_file;
    const char *function_name;
    int line_number;
    DetScatStatus status_code;
    char message[DETSCAT_DIAG_MSG_MAX];
} DetScatDiagnose;

#define DETSCAT_SET_DIAGNOSE(diag, status, fmt, ...)                        \
    do {                                                                    \
        (diag).status_code = (status);                                      \
        snprintf((diag).message, sizeof((diag).message), fmt, __VA_ARGS__); \
        (diag).function_name = __func__;                                    \
        (diag).source_file = __FILE__;                                      \
        (diag).line_number = __LINE__;                                      \
    } while (0)

const char *detscat_diag_status_to_str(DetScatStatus status); 
#endif  // DETSCAT_DIAG_H
