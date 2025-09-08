#include "detscat_error.h"

#include "detscat.h"

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// --- Internal helpers (SHARED) ---
void detscat_error_set(DetScatError *err, const char *src, const char *func,
                      int lineno, DetScatStatus status, const char *fmt, ...) {
    if (!err) return;

    assert(status >= 0 && status < DETSCAT_STATUS_COUNT);
    if (status < 0 || status >= DETSCAT_STATUS_COUNT) {
        err->status = DETSCAT_ERR_UNKNOWN;
    } else {
        err->status = status;
    }

    err->file_name = src ? src : "";
    err->function_name = func ? func : "";
    err->line_number = lineno ? lineno : 0;

    if (fmt) {
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(err->message, sizeof(err->message), fmt, args);
        va_end(args);

        if (n < 0) {
            err->status = DETSCAT_ERR_MSG_ENCODE;
            err->message[0] = '\0';
        } else if ((size_t)n >= sizeof(err->message)) {
            const char suffix[] = "...[truncated]";
            size_t msg_len = sizeof(err->message) - 1;
            size_t suffix_len = sizeof(suffix) - 1;
            if (msg_len > suffix_len) {
                memcpy(err->message + msg_len - suffix_len, suffix, suffix_len);
            }
            err->message[msg_len] = '\0';
        }
    } else {
        err->message[0] = '\0';
    }
}

const char *detscat_error_status_to_str(DetScatStatus status) {
    assert(status >= 0 && status < DETSCAT_STATUS_COUNT);
    switch (status) {
#define X(name, str)     \
    case DETSCAT_##name: \
        return str;
        DETSCAT_STATUS_LIST
#undef X
        default:
            return "Unknown status";
    }
}


// --- Public API ---
DetScatError *detscat_error_create(void) {
    DetScatError *err = calloc(1, sizeof(*err));
    if (!err) return NULL;

    err->status = DETSCAT_OK;

    return err;
}

void detscat_error_destroy(DetScatError **err) {
    if (!err || !*err) return;

    free(*err);
    *err = NULL;
}

DetScatStatus detscat_error_status(const DetScatError *err) {
    assert(err != NULL);
    if (!err) return DETSCAT_ERR_UNKNOWN; 

    return err->status;
}

const char *detscat_error_message(const DetScatError *err) {
    assert(err != NULL);
    if (!err) return "";

    return err->message;
} 
