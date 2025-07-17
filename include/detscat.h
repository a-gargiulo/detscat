#ifndef DETSCAT_H
#define DETSCAT_H

#define DETSCAT_ERR_MSG_MAX 256

#define DETSCAT_SET_DIAGNOSE(diag, status_val, msg_fmt, ...)                        \
    do {                                                                            \
        (diag).status = (status_val);                                               \
        snprintf((diag).err_msg, sizeof((diag).err_msg), (msg_fmt), ##__VA_ARGS__); \
        (diag).function = __func__;                                                 \
        (diag).file = __FILE__;                                                     \
        (diag).line = __LINE__;                                                     \
    } while (0)

typedef enum {
    DETSCAT_OK = 0,
    DETSCAT_ERR_COMMAND_LINE_ARGS,
    DETSCAT_ERR_FILE_PARSING
} DetScatStatus;

typedef struct {
    DetScatStatus status;
    char err_msg[DETSCAT_ERR_MSG_MAX];
    const char *file;
    const char *function;
    int line;
} DetScatDiagnose;

void detscat_run(int argc, char **argv, DetScatDiagnose *diagnose);

// double detscat_calculate_incident_field_strength(double d, double E_p_mj, double tau_p_ns);

#endif  //  DETSCAT_H
