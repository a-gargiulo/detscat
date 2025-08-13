#ifndef DETSCAT_H
#define DETSCAT_H

#include <stdarg.h>

#define DETSCAT_ERR_MSG_MAX 256
#define DETSCAT_PATH_MAX 1024 

#define DETSCAT_SET_DIAGNOSE(diag, status_val, msg_fmt, ...)                        \
    do {                                                                            \
        (diag).status = (status_val);                                               \
        snprintf((diag).err_msg, sizeof((diag).err_msg), (msg_fmt), __VA_ARGS__);   \
        (diag).func = __func__;                                                     \
        (diag).file = __FILE__;                                                     \
        (diag).line = __LINE__;                                                     \
    } while (0)

typedef enum {
    DETSCAT_OK = 0,
    DETSCAT_ERR_MISSING_CMD_ARG,
    DETSCAT_ERR_FILE_PARSING,
    DETSCAT_ERR_ALLOC,
    DETSCAT_ERR_LOOKUP
} DetScatStatus;

typedef struct {
    const char *file;
    const char *func;
    int line;
    DetScatStatus status;
    char err_msg[DETSCAT_ERR_MSG_MAX];
} DetScatDiagnose;

void detscat_run(int argc, char **argv, DetScatDiagnose *diagnose);

void detscat_error(const char *fnct, int line, const char *file, const char *frmt, ...);

void detscat_info(const char *frmt, ...);

// double detscat_calculate_incident_field_strength(double d, double E_p_mj, double tau_p_ns);

#endif  //  DETSCAT_H
