#include <stdio.h>

#include "detscat.h"

int main(int argc, char **argv) {
    int exit_code = 0;

    if (argc < 2 || !*argv[1]) {
        printf("[\033[91mERROR\033[0m]: "
                "\033[93mUsage\033[0m: "
                "\033[90m%s <path_to_cfg_file>\033[0m\n", argv[0]);
        return 1;
    }
    const char *cfg_file_path = argv[1];

    DetScatError *err = detscat_error_create();
    DetScat *detscat = NULL;

    if (!detscat_init(err)) {
        detscat_log_error(err);
        exit_code = 1;
        goto cleanup;
    }
    detscat_log(DETSCAT_INFO, "DetScat initialized successfully");

    detscat = detscat_create(cfg_file_path, err);
    if (!detscat) {
        detscat_log_error(err);
        exit_code = 1;
        goto cleanup;
    }

    if (!detscat_load_data(detscat, err)) {
        detscat_log_error(err);
        exit_code = 1;
        goto cleanup;
    }

    if (!detscat_setup_camera(detscat, err)) {
        detscat_log_error(err);
        exit_code = 1;
        goto cleanup;
    }

    if (!detscat_simulation_run(detscat, err)) {
        detscat_log_error(err);
        exit_code = 1;
        goto cleanup;
    }

   if (!detscat_construct_image(detscat, err)) { 
        detscat_log_error(err);
        exit_code = 1;
        goto cleanup;
    }

cleanup:
    detscat_destroy(&detscat);
    detscat_error_destroy(&err);

    if (exit_code == 0)
        detscat_log(DETSCAT_INFO, "DetScat terminated successfully");

    detscat_shutdown();
    return exit_code;
}
