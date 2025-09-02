#define DETSCAT_ENABLE_LOGGING
#include "detscat.h"

#include <stdio.h>


int main(int argc, char **argv) {
    if (argc < 2 || argv[1][0] == '\0') {
        printf("[ERROR]: Missing or empty configuration file path. "
               "Usage: %s <path_to_configuration_file>\n",
               argv[0]);
        return 1;
    }
    const char *config_path = argv[1];

    DetScatDiagnose diag = {0};

    detscat_init();
    detscat_log_info("DetScat initialized successfully");

    DetScat detscat = {0};
    if (!detscat_load_data(config_path, &detscat, &diag)) {
        detscat_log_error_diagnose(&diag);
        return 1;
    }
    detscat_log_debug("Parsed configuration data:");
    detscat_print_cfg(detscat.cfg);


    detscat_data_free(&detscat);
    detscat_terminate();
    detscat_log_info("DetScat terminated successfully");
    return 0;
}
