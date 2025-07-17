#include "detscat.h"


#include "detscat_config.h"


void detscat_parse_config_file(char** argv, DetScatConfig *config, DetScatDiagnose *diagnose) {
    DetScatConfigParser *cfg_parser = detscat_config_parser_create(argv[1]);
    if (!cfg_parser) {
        DETSCAT_SET_DIAGNOSE(
            *diagnose, DETSCAT_ERR_FILE_PARSING, "%s",
            "Could not initialize the configuration file parser."
        );
        return;
    }

    if (!detscat_config_parser_parse(cfg_parser, config)) {
        DETSCAT_SET_DIAGNOSE(
            *diagnose, DETSCAT_ERR_FILE_PARSING,
            "While parsing '%s': %s", argv[1], cfg_parser->error_message
        );
        detscat_config_parser_free(cfg_parser);
        return;
    }
    detscat_config_parser_free(cfg_parser);

    printf("[\033[94mINFO\033[0m]: Successfully parsed '%s'.\n", argv[1]);

    return;
}


void detscat_run(int argc, char **argv, DetScatDiagnose *diagnose) {
    DetScatConfig config;

    if (argc < 2) {
        DETSCAT_SET_DIAGNOSE(
            *diagnose, DETSCAT_ERR_COMMAND_LINE_ARGS, "%s",
            "Missing command-line argument. Please specify the configuration file.");
        return;
    }
    printf("[\033[94mINFO\033[0m]: DetScat initialized successfully.\n");

    detscat_parse_config_file(argv, &config, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

    return;
}


// double detscat_calculate_incident_field_strength(double d, double E_p_mj, double tau_p_ns) {
//     double A = d * d * DETSCAT_CONST_MM2M * DETSCAT_CONST_MM2M * M_PI / 4.0;
//     double I = E_p_mj * DETSCAT_CONST_MJ2J / tau_p_ns / DETSCAT_CONST_NS2S / A;
//     return sqrt(2 * I / DETSCAT_CONST_C_MS / DETSCAT_CONST_EPS0_F_M);
// }
