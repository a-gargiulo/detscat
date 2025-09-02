#include "detscat_diag.h"

#include <stdarg.h>
#include <stdio.h>

void detscat_diag_set(DetScatDiagnose *diag, const char *src, const char *func,
                      int lineno, DetScatStatus status, const char *fmt, ...) {
    if (!diag) return;

    diag->status = status;
    diag->file_name = src ? src : "";
    diag->function_name = func ? func : "";
    diag->line_number = lineno;

    if (fmt) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(diag->error_message, sizeof(diag->error_message), fmt, args);
        va_end(args);
    } else {
        diag->error_message[0] = '\0';
    }
}

const char *detscat_diag_status_to_str(DetScatStatus status) {
    switch (status) {
#define X(name, str) case DETSCAT_##name: return str;
        DETSCAT_STATUS_LIST
#undef X
        default: return "Unknown status";
    }
}

const char *detscat_diag_status_repr(DetScatStatus status) {
    switch (status) {
#define X(name, str) case DETSCAT_##name: return "DETSCAT_" #name;
        DETSCAT_STATUS_LIST
#undef X
        default: return "DETSCAT_UNKNOWN";
    }
}
