#include "detscat_error.h"

#include "detscat.h"

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// =============================================================================
// INTERNAL Helpers
// =============================================================================
static inline bool ds_status_is_valid(DetScatStatus status) {
    switch(status) {
#define X(name, str) case DS_##name:
        DS_STATUS_LIST
#undef X
            return true;
        default:
            return false;
    }
}


// =============================================================================
// SHARED API 
// =============================================================================
void ds_error_set(DetScatError *err, const char *src, const char *func,
                  int lineno, DetScatStatus status, const char *fmt, ...) {
    if (!err) return;

    err->status = ds_status_is_valid(status) ? status : DS_UNKNOWN;
    err->file_name = src ? src : "";
    err->function_name = func ? func : "";
    err->line_number = lineno ? lineno : 0;

    if (fmt) {
        va_list args;
        va_start(args, fmt);
        int n = vsnprintf(err->message, sizeof(err->message), fmt, args);
        va_end(args);

        if (n < 0) {
            err->status = DS_ERR_MSG_ENCODE;
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

const char *ds_error_status_to_str(DetScatStatus status) {
    assert(ds_status_is_valid(status));
    switch (status) {
#define X(name, str) case DS_##name: return str;
        DS_STATUS_LIST
#undef X
        default:
            return "Unknown status";
    }
}


// =============================================================================
// PUBLIC API 
// =============================================================================
DetScatError *ds_error_create(void) {
    DetScatError *err = calloc(1, sizeof(*err));
    if (!err) return NULL;

    err->status = DS_OK;

    return err;
}

void ds_error_destroy(DetScatError **err) {
    if (!err || !*err) return;

    free(*err);
    *err = NULL;
}

DetScatStatus ds_error_status(const DetScatError *err) {
    assert(err);
    return err->status;
}

const char *ds_error_message(const DetScatError *err) {
    assert(err);
    return err->message;
} 
