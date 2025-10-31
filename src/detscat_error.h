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


void ds_error_set(DetScatError *err, const char *src, const char *func,
                  int lineno, DetScatStatus status, const char *fmt, ...);

#define DS_SET_ERROR(err, status, fmt, ...)                                    \
    do {                                                                       \
        if ((err)) {                                                           \
            ds_error_set((err), __FILE__, __func__, __LINE__, (status), (fmt), \
                         ##__VA_ARGS__);                                       \
        }                                                                      \
    } while (0)

const char *ds_error_status_to_str(DetScatStatus status);

#endif  // DETSCAT_ERROR_H
