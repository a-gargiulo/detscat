#include "detscat.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "detscat_camera.h"
#include "detscat_config.h"
#include "detscat_ddscat_util.h"
#include "detscat_particles.h"


static void detscat_print_banner(void) {
    printf("************** Welcome to **************\n");
    printf( 
        "  _____       _    _____           _    \n"
        " |  __ \\     | |  / ____|         | |   \n"
        " | |  | | ___| |_| (___   ___ __ _| |_  \n"
        " | |  | |/ _ \\ __|\\___ \\ / __/ _` | __| \n"
        " | |__| |  __/ |_ ____) | (_| (_| | |_  \n"
        " |_____/ \\___|\\__|_____/ \\___\\__,_|\\__| \n"
        "                                        \n"
    );
    printf("\n");
    printf("(C) Aldo Gargiulo 2025\n\n");
    printf("****************************************\n");
    return;
}

static void detscat_vprint(const char *frmt, va_list args) {
    while (*frmt) {
        if (*frmt == '%' && *(frmt + 1)) {
            frmt++;
            switch (*frmt) {
                case 'd':
                    printf("%d", va_arg(args, int));
                    break;
                case 'c':
                    putchar((char)va_arg(args, int));
                    break;
                case 'f':
                    printf("%f", va_arg(args, double));
                    break;
                case 's':
                    printf("%s", va_arg(args, const char *));
                    break;
                case 'l':
                    if (*(frmt + 1) == 'f') {
                        frmt += 2;
                        printf("%lf", va_arg(args, double));
                        continue;
                    } else if (*(frmt + 1) == 'd') {
                        frmt += 2;
                        printf("%ld", va_arg(args, long int));
                        continue;
                    }
                    putchar('%');
                    putchar('l');
                    break;
                case 'z':
                    if (*(frmt + 1) == 'u') {
                        frmt += 2;
                        printf("%zu", va_arg(args, size_t));
                        continue;
                    }
                    putchar('%');
                    putchar('z');
                    break;
                default:
                    putchar('%');
                    putchar(*frmt);
            }
        } else {
            putchar(*frmt);
        }
        frmt++;
    }
}

void detscat_error(const char *fnct, int line, const char *file, const char *frmt, ...) {
    printf("[\033[91mERROR\033[0m]: From function \033[95m%s\033[0m on \033[95mline %d\033[0m in file \033[95m%s\033[0m: ", fnct,
           line, file);
    va_list args;
    va_start(args, frmt);
    detscat_vprint(frmt, args);
    putchar('\n');
    va_end(args);
    return;
}

void detscat_info(const char *frmt, ...) {
    printf("[\033[94mINFO\033[0m]: ");
    va_list args;
    va_start(args, frmt);
    detscat_vprint(frmt, args);
    putchar('\n');
    va_end(args);
    return;
}

static void detscat_parse_config_file(char **argv, DetScatConfig *config, DetScatDiagnose *diagnose) {
    DetScatConfigParser *cfg_parser = detscat_config_parser_create(argv[1]);
    if (!cfg_parser) {
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING, "%s",
                             "Could not initialize the configuration file parser.");
        return;
    }

    if (!detscat_config_parser_parse(cfg_parser, config)) {
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING, "While parsing '%s': %s", argv[1],
                             cfg_parser->error_message);
        detscat_config_parser_free(cfg_parser);
        return;
    }
    detscat_config_parser_free(cfg_parser);

    detscat_info("Successfully parsed '%s'.", argv[1]);
    return;
}


static void detscat_parse_particles_definition_file(DetScatParticlesData *particles_data, DetScatConfig *cfg, DetScatDiagnose *diagnose)
{
    DetScatParticlesParser *particles_parser = detscat_particles_parser_create(cfg->particles_definition_file); 
    if (!particles_parser) {
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING, "%s",
                             "Could not initialize the particles definition file parser.");
        return;
    }

    if (!detscat_particles_parser_parse(particles_parser, particles_data)) {
        DETSCAT_SET_DIAGNOSE(*diagnose, DETSCAT_ERR_FILE_PARSING, "While parsing '%s': %s", cfg->particles_definition_file, particles_parser->error_message);
        detscat_particles_parser_free(particles_parser);
        detscat_particles_free(particles_data);
        return;
    }
    detscat_particles_parser_free(particles_parser);

    detscat_info("Successfully parsed '%s'.", cfg->particles_definition_file);
    return;
}

static void detscat_fetch_ddscat_data(DdscatPar ***par, Fmat ***fmat, DetScatParticlesData *particles_data, DetScatDiagnose *diagnose) {

    *par = malloc(particles_data->n_definitions * sizeof(DdscatPar*));
    *fmat = malloc(particles_data->n_definitions * sizeof(Fmat*));

    if (!*par || !*fmat) {
        *par = NULL;
        *fmat = NULL;
        DETSCAT_SET_DIAGNOSE(
            *diagnose, DETSCAT_ERR_ALLOC, "%s",
            "Could not allocate DDSCAT data containers.");
        return;
    }

    for (size_t i = 0; i < particles_data->n_definitions; ++i) {
        DetScatDdscatUtilStatus status;
        
        char par_file_path[DETSCAT_PATH_MAX];
        strcpy(par_file_path, particles_data->definitions[i].data_dir);

        if (par_file_path[strlen(par_file_path) -1] == '/')
            strcat(par_file_path, "ddscat.par");
        else
            strcat(par_file_path, "/ddscat.par");
        
        status = detscat_ddscat_util_parse_par_file(par_file_path, (*par)[i]);
        if (status != DETSCAT_DDSCAT_UTIL_OK) {
            DETSCAT_SET_DIAGNOSE(
                *diagnose, DETSCAT_ERR_FILE_PARSING, 
                "Could not parse the DDSCAT parameter file '%s' for particle type '%s'. Parser failed with error code: %d.",
                par_file_path, particles_data->definitions[i].id, status);
            free(*fmat);
            *fmat = NULL;
        }

        
    }

    return fmat;
}



void detscat_run(int argc, char **argv, DetScatDiagnose *diagnose) {
    detscat_print_banner();

    DetScatConfig config;
    DetScatParticlesData particles_data = {0};


    // Camera *camera = detscat_camera_create(&config);
    // Image *image = detscat_camera_image_create(camera->width, camera->height);

    // VALIDATE COMMAND-LINE INPUT
    if (argc < 2) {
        DETSCAT_SET_DIAGNOSE(
            *diagnose, DETSCAT_ERR_COMMAND_LINE_ARGS, "%s",
            "Missing command-line argument. Please specify the configuration file.");
        return;
    }
    detscat_info("DetScat initialized successfully.");

    // PARSE CONFIG FILE
    detscat_parse_config_file(argv, &config, diagnose);
    if (diagnose->status != DETSCAT_OK) return;

    // PARSE PARTICLES FILE
    detscat_parse_particles_definition_file(&particles_data, &config, diagnose);
    if (diagnose->status != DETSCAT_OK) return;


    // COLLECT DDSCAT DATA
    DdscatPar **par;
    Fmat **fmat;
    detscat_fetch_ddscat_data(&par, &fmat, &particles_data, diagnose);

    detscat_particles_free(&particles_data);
    return;
}

// double detscat_calculate_incident_field_strength(double d, double E_p_mj, double tau_p_ns) {
//     double A = d * d * DETSCAT_CONST_MM2M * DETSCAT_CONST_MM2M * M_PI / 4.0;
//     double I = E_p_mj * DETSCAT_CONST_MJ2J / tau_p_ns / DETSCAT_CONST_NS2S / A;
//     return sqrt(2 * I / DETSCAT_CONST_C_MS / DETSCAT_CONST_EPS0_F_M);
// }
