#include "detscat.h"

int main(int argc, char **argv) {
    DetScatDiagnose diagnose = {0};

    detscat_run(argc, argv, &diagnose);
    if (diagnose.status != DETSCAT_OK) {
        detscat_error(diagnose.function, diagnose.line, diagnose.file, diagnose.err_msg);
        return 1;
    }

    detscat_info("DetScat completed successfully!");

    return 0;
}
