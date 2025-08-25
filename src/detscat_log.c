#include "detscat_log.h"

#include <stdarg.h>
#include <stdio.h>


void detscat_log_debug(const char *fmt, ...) {
    printf("[\033[92mDEBUG\033[0m]: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
    fflush(stdout);
}

void detscat_log_info(const char *fmt, ...) {
    printf("[\033[94mINFO\033[0m]: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
    fflush(stdout);
}

void detscat_log_warning(const char *fmt, ...) {
    printf("[\033[93mWARNING\033[0m]: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
    fflush(stdout);
}

void detscat_log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[\033[91mERROR\033[0m]: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    fflush(stderr);
}

void detscat_log_error_diagnose(const DetScatDiagnose *diag) {
    if (!diag) return;

    fprintf(
        stderr,
        "[\033[91mERROR\033[0m] "
        "\033[90m%s:%d\033[0m "
        "(\033[96m%s\033[0m) "
        "status=\033[93m%s\033[0m\n"
        "        message: %s\n",
        diag->source_file,
        diag->line_number,
        diag->function_name,
        detscat_diag_status_to_str(diag->status_code),
        diag->message);
}
