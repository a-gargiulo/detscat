#include <stdio.h>

#include "detscat_config.h"
#include "detscat_particles.h"

// #include <stdlib.h>
// #include <string.h>
// #include <errno.h>

// #include "ddscat.h"
// #include "mymath.h"

DetScatConfig config;

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
                "[ERROR]: Missing command-line argument. Please specify the 'config file' path.\n");
        return 1;
    }

    // PARSE CONFIG FILE
    DetScatConfigParser *cfg_parser = detscat_config_parser_create(argv[1]);
    if (!cfg_parser) {
        fprintf(stderr, "[ERROR]: Could not initialize the config parser.\n");
        return 1;
    }

    if (!detscat_config_parser_parse(cfg_parser, &config)) {
        fprintf(stderr, "[ERROR]: While parsing '%s': %s\n", argv[1], cfg_parser->error_message);
        detscat_config_parser_free(cfg_parser);
        return 1;
    }
    detscat_config_parser_free(cfg_parser);

    // PARSE PARTICLES
    DetScatParticlesParser *particles_parser =
        detscat_particles_parser_create(config.particles_definition_file);
    if (!particles_parser) {
        fprintf(stderr, "[ERROR]: Could not initialize particles parser.\n");
        return 1;
    }

    DetScatParticlesData particles_data = {0};

    if (!detscat_particles_parser_parse(particles_parser, &particles_data)) {
        fprintf(stderr, "[ERROR]: While parsing '%s': %s\n", config.particles_definition_file,
                particles_parser->error_message);
        detscat_particles_parser_free(particles_parser);
        detscat_particles_free(&particles_data);
        return 1;
    }
    detscat_particles_parser_free(particles_parser);

    // INSERT MORE CODE HERE

    detscat_particles_free(&particles_data);
    return 0;
}

// Allocate FMATS for each type

// Main Loop

// const char *root = "../../ddscat/ELLIPSOID";

// DdscatError ddscat_err;

// char par_file_path[512];
// char fml_file_path[512];

// sprintf(par_file_path, "%s/ddscat.par", root);
// sprintf(fml_file_path, "%s/w000r000k000.fml", root);

// Par par;
// Fmat *fmat;
// Smat *smat;

// ddscat_err = ddscat_parse_par_file(par_file_path, &par);
// if (ddscat_err != DDSCAT_OK) {
//     // TODO: report error
//     printf("ERROR %d\n", ddscat_err);
//     return 1;
// }

// ddscat_err = ddscat_parse_fml_file(fml_file_path, &par, &fmat);
// if (ddscat_err != DDSCAT_OK) {
//     // TODO: report error
//     printf("ERROR %d\n", ddscat_err);
//     return 1;
// }

// ddscat_err = ddscat_calculate_scatmat(&par, fmat, &smat);
// if (ddscat_err != DDSCAT_OK) {
//     // TODO: report error
//     printf("ERROR %d\n", ddscat_err);
//     return 1;
// }

// // WRITE DATA
// FILE *sfile = fopen("sdata.txt", "w");
// if (!sfile) {
//     fprintf(stderr, "Error opening file for writing: %s\n", strerror(errno));
//     ddscat_err = DDSCAT_ERR_CANNOT_OPEN_FILE;
//     goto cleanup;
// }

// for (size_t i = 0; i < par.nplanes; ++i) {

//     double *theta = malloc(smat[i].n * sizeof(double));
//     if (!theta) {
//         printf("ERROR\n");
//         fclose(sfile);
//         goto cleanup;
//     }

//     for (size_t m = 0; m < smat[i].n; ++m) {
//         theta[m] = par.planes[i][1] + m * par.planes[i][3];
//     }
//     theta[smat[i].n - 1] = par.planes[i][2];

//     for (size_t j = 0; j < smat[i].n; ++j) {
//         fprintf(sfile, "%22.15E %22.15E %22.15E %22.15E %22.15E %22.15E %22.15E %22.15E %22.15E
//         %22.15E\n",
//                 theta[j], par.planes[i][0],
//                 smat[i].S1[j].re, smat[i].S1[j].im,
//                 smat[i].S2[j].re, smat[i].S2[j].im,
//                 smat[i].S3[j].re, smat[i].S3[j].im,
//                 smat[i].S4[j].re, smat[i].S4[j].im);
//     }

//     free(theta);
//     theta = NULL;
// }

// // PRINT DATA
// // for (size_t i = 0; i < par.nplanes; ++i) {
// //     printf("Plane %zu: %lf %lf %lf %lf\n", i+1,
// //            par.planes[i][0], par.planes[i][1],
// //            par.planes[i][2], par.planes[i][3]);

// //     double *theta = (double *)malloc(fmat[i].n * sizeof(double));
// //     if (!theta) {
// //         printf("ERROR\n");
// //         break;
// //     }

// //     for (size_t m = 0; m < fmat[i].n; ++m) {
// //         theta[m] = par.planes[i][1] + m * par.planes[i][3];
// //     }
// //     theta[fmat[i].n - 1] = par.planes[i][2];

// //     for (size_t j = 0; j < fmat[i].n; ++j) {
// //         printf("%.1lf %.1lf %.3E %.3E %.3E %.3E %.3E %.3E %.3E %.3E\n",
// //                theta[j], par.planes[i][0],
// //                fmat[i].f11[j].re, fmat[i].f11[j].im,
// //                fmat[i].f21[j].re, fmat[i].f21[j].im,
// //                fmat[i].f12[j].re, fmat[i].f12[j].im,
// //                fmat[i].f22[j].re, fmat[i].f22[j].im);
// //     }

// //     free(theta);
// //     theta = NULL;
// // }

// fclose(sfile);
// goto cleanup;

// cleanup:
// for (size_t i = 0; i < par.nplanes; ++i) {
//     free(fmat[i].f11);
//     free(fmat[i].f21);
//     free(fmat[i].f12);
//     free(fmat[i].f22);
//     free(smat[i].S1);
//     free(smat[i].S2);
//     free(smat[i].S3);
//     free(smat[i].S4);
// }
// free(fmat);
// free(smat);
// fmat = NULL;
// smat = NULL;

// for (size_t i = 0; i < par.ncomp; ++i) {
//     free(par.comp[i]);
// }
// free(par.comp);
// free(par.planes);

// if (ddscat_err != DDSCAT_OK) {
//     return 1;
// }

// printf("SUCCESS!\n");
// return 0;
