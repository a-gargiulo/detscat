#ifndef DETSCAT_ERROR_H
#define DETSCAT_ERROR_H

#include "detscat.h"

struct DetScatError {
    const char *file_name;
    const char *function_name;
    int line_number;
    DetScatStatus status;
    char message[256];
};

#define DETSCAT_STATUS_LIST                                    \
    X(OK, "OK")                                                \
    X(ERR_CMD_ARG, "Invalid or missing command-line argument") \
    X(ERR_INVALID_ARG, "Invalid argument")                     \
    X(ERR_KEY_LOOKUP, "Key lookup error")                      \
    X(ERR_MEMORY, "Memory allocation error")                   \
    X(ERR_OVERFLOW, "Overflow error")                          \
    X(ERR_PARSE, "Parsing error")                              \
    X(ERR_RANGE, "Argument out of range")                      \
    X(ERR_UNDERFLOW, "Underflow error")

void detscat_error_set(DetScatError *err, const char *src, const char *func,
                       int lineno, DetScatStatus status, const char *fmt, ...);

#define DETSCAT_SET_ERROR(err, status, fmt, ...)                               \
    do {                                                                       \
        if ((err)) {                                                           \
            detscat_error_set(err, __FILE__, __func__, __LINE__, status, fmt,  \
                              ##__VA_ARGS__);                                  \
        }                                                                      \
    } while (0)

const char *detscat_error_status_to_str(DetScatStatus status);

#endif  // DETSCAT_ERROR_H
