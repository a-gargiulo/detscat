#include "detscat_log.h"

#include "detscat.h"

#include "detscat_error.h"

#include <assert.h>
#include <omp.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>


static omp_lock_t log_lock;
static int log_lock_initialized = 0;


// =============================================================================
// INTERNAL Helpers
// =============================================================================
static inline bool ds_log_level_is_valid(DetScatLogLevel level) {
    switch(level) {
#define X(name) case DS_##name:
        DS_LOG_LEVEL_LIST
#undef X
            return true;
        default:
            return false;
    }
}

static void ds_log_vprint(FILE *out, const char *prefix, const char *fmt,
                          va_list args) {
    if (!fmt || !*fmt) return;
    out = !out ? stdout : out;
    assert(prefix != NULL && *prefix);

    omp_set_lock(&log_lock);
    fprintf(out, "%s", prefix);
    vfprintf(out, fmt, args);
    fprintf(out, "\n");
    if (out == stdout || out == stderr) fflush(out);
    omp_unset_lock(&log_lock);
}

static inline void ds_log_prefix(char *buf, size_t bufsize, int level) {
    assert(buf && bufsize > 0);
    assert(ds_log_level_is_valid(level));

    const char *color, *label;
    switch(level) {
        case DS_DEBUG:
            color = "\033[92m";
            label = "DEBUG";
            break;
        case DS_INFO:
            color = "\033[94m";
            label = "INFO";
            break;
        case DS_WARNING:
            color = "\033[93m";
            label = "WARNING";
            break;
        case DS_ERROR:
            color = "\033[91m";
            label = "ERROR";
            break;
        default:
            color = "\033[0m";
            label = "LOG";
            break;
    }
    snprintf(buf, bufsize, "[%s%s\033[0m]: ", color, label);
}


// =============================================================================
// SHARED API
// =============================================================================
void ds_log_init_lock(void) {
    if (!log_lock_initialized) {
        omp_init_lock(&log_lock);
        log_lock_initialized = 1;
    }
}

void ds_log_destroy_lock(void) {
    if (log_lock_initialized) {
        omp_destroy_lock(&log_lock);
        log_lock_initialized = 0;
    }
}


// =============================================================================
// PUBLIC API
// =============================================================================
void ds_log(DetScatLogLevel level, const char *fmt, ...) {
    char prefix[32];
    ds_log_prefix(prefix, sizeof(prefix), level);

    va_list args;
    va_start(args, fmt);
    ds_log_vprint(stderr, prefix, fmt, args);
    va_end(args);
}

void ds_log_error(const DetScatError *err) {
    if (!err) return;

    omp_set_lock(&log_lock);

    fprintf(stderr,
            "[\033[91mERROR\033[0m]: \033[90m%s:%d\033[0m (\033[96m%s\033[0m): "
            "\033[93m%s\033[0m\n\n"
            "         Message:\n"
            "                     %s\n",
            err->file_name, err->line_number, err->function_name,
            ds_error_status_to_str(err->status), err->message);

    fflush(stderr);
    omp_unset_lock(&log_lock);
}
