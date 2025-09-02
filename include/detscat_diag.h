#ifndef DETSCAT_DIAG_H
#define DETSCAT_DIAG_H

#include <stdbool.h>
#include <stddef.h>


#define DETSCAT_STATUS_LIST \
    X(OK, "OK") \
    X(ERR_CMD_ARGUMENT, "Invalid or missing command-line argument") \
    X(ERR_INVALID_ARGUMENT, "Invalid argument") \
    X(ERR_LOOKUP_KEY, "Key lookup error") \
    X(ERR_MEMORY, "Memory allocation error") \
    X(ERR_OVERFLOW, "Overflow error") \
    X(ERR_PARSE, "Parsing error") \
    X(ERR_RANGE, "Argument out of range") \
    X(ERR_UNDERFLOW, "Underflow error")

typedef enum {
#define X(name, str) DETSCAT_##name,
    DETSCAT_STATUS_LIST
#undef X
    DETSCAT_STATUS_COUNT
} DetScatStatus;

typedef struct {
    const char *file_name;
    const char *function_name;
    int line_number;
    DetScatStatus status;
    char error_message[256];
} DetScatDiagnose;

void detscat_diag_set(DetScatDiagnose *diag, const char *src, const char *func,
                      int lineno, DetScatStatus status, const char *fmt, ...);
const char *detscat_diag_status_to_str(DetScatStatus status);
const char *detscat_diag_status_repr(DetScatStatus status);

#define DETSCAT_SET_DIAG(diag, status, fmt, ...)                          \
    do {                                                                      \
        if ((diag)) {                                                         \
            detscat_diag_set(diag, __FILE__, __func__, __LINE__, status, fmt, \
##__VA_ARGS__);                                    \
        }                                                                     \
    } while (0)

#endif  // DETSCAT_DIAG_H
