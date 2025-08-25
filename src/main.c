#define DETSCAT_ENABLE_LOGGING
#include "detscat.h"

int main(int argc, char **argv) {
    DetScatDiagnose diag = {0};

    if (detscat_run(argc, argv, &diag) != DETSCAT_OK) {
        detscat_log_error_diagnose(&diag);
        return 1;
    }

    detscat_log_info("Program finished successfully");
    return 0;
}
