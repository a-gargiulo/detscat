#include "detscat_log.h"

#include "detscat_diag.h"

#include <assert.h>
#include <omp.h>
#include <stdarg.h>
#include <stdio.h>

static omp_lock_t log_lock;
static int log_lock_initialized = 0;

void detscat_log_init_lock(void) {
    if (!log_lock_initialized) {
        omp_init_lock(&log_lock);
        log_lock_initialized = 1;
    }
}

void detscat_log_terminate_lock(void) {
    if (log_lock_initialized) {
        omp_destroy_lock(&log_lock);
        log_lock_initialized = 0;
    }
}

static void detscat_log_vprint(FILE *out, const char *prefix, const char *fmt,
                               va_list args) {
    if (!fmt || !*fmt) return;

    omp_set_lock(&log_lock);

    fprintf(out, "%s", prefix);
    vfprintf(out, fmt, args);
    fprintf(out, "\n");
    fflush(out);

    omp_unset_lock(&log_lock);
}


void detscat_log_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detscat_log_vprint(stdout, "[\033[92mDEBUG\033[0m]: ", fmt, args);
    va_end(args);
}

void detscat_log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detscat_log_vprint(stdout, "[\033[94mINFO\033[0m]: ", fmt, args);
    va_end(args);
}

void detscat_log_warning(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detscat_log_vprint(stdout, "[\033[93mWARNING\033[0m]: ", fmt, args);
    va_end(args);
}

void detscat_log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    detscat_log_vprint(stderr, "[\033[91mERROR\033[0m]: ", fmt, args);
    va_end(args);
}

void detscat_log_error_diagnose(const DetScatDiagnose *diag) {
    if (!diag) return;

    omp_set_lock(&log_lock);

    fprintf(stderr,
            "[\033[91mERROR\033[0m] \033[90m%s:%d\033[0m (\033[96m%s\033[0m): "
            "\033[93m%s\033[0m\n\n"
            "        Message:\n"
            "                    %s\n",
            diag->file_name, diag->line_number, diag->function_name,
            detscat_diag_status_to_str(diag->status), diag->error_message);
    fflush(stderr);

    omp_unset_lock(&log_lock);
}
