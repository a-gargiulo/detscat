#ifndef DETSCAT_LOG_H
#define DETSCAT_LOG_H

#include "detscat_diag.h"

void detscat_log_debug(const char *fmt, ...);

void detscat_log_info(const char *fmt, ...);

void detscat_log_warning(const char *fmt, ...);

void detscat_log_error(const char *fmt, ...);

void detscat_log_error_diagnose(const DetScatDiagnose *diag);

#endif  // DETSCAT_LOG_H
