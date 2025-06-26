#include <stdio.h>
#include <stdlib.h>

#include "ddscat.h"
#include "mymath.h"


int main(void) {


    const char *root = "./sphere";

    DdscatError ddscat_err;

    char par_file_path[512];
    char fml_file_path[512];

    sprintf(par_file_path, "%s/ddscat.par", root);
    sprintf(fml_file_path, "%s/w000r000k000.fml", root);


    Par par;
    Fmat *fmat;

    ddscat_err = ddscat_parse_par_file(par_file_path, &par);
    if (ddscat_err != DDSCAT_OK) {
        // TODO: report error
        printf("ERROR %d\n", ddscat_err);
        return 1;
    }

    ddscat_err = ddscat_parse_fml_file(fml_file_path, &par, &fmat);
    if (ddscat_err != DDSCAT_OK) {
        // TODO: report error
        printf("ERROR %d\n", ddscat_err);
        return 1;
    }

    for (size_t i = 0; i < par.nplanes; ++i) {
        printf("Plane %zu: %lf %lf %lf %lf\n", i+1, 
               par.planes[i][0], par.planes[i][1], 
               par.planes[i][2], par.planes[i][3]);

        double *theta = (double *)malloc(fmat[i].n * sizeof(double));
        if (!theta) {
            printf("ERROR\n");
            break;
        }

        for (size_t m = 0; m < fmat[i].n; ++m) {
            theta[m] = par.planes[i][1] + m * par.planes[i][3];
        }
        theta[fmat[i].n - 1] = par.planes[i][2];

        for (size_t j = 0; j < fmat[i].n; ++j) {
            printf("%.1lf %.1lf %.3E %.3E %.3E %.3E %.3E %.3E %.3E %.3E\n", 
                   theta[j], par.planes[i][0],
                   fmat[i].f11[j].re, fmat[i].f11[j].im,
                   fmat[i].f21[j].re, fmat[i].f21[j].im,
                   fmat[i].f12[j].re, fmat[i].f12[j].im,
                   fmat[i].f22[j].re, fmat[i].f22[j].im);
        }

        free(theta);
        theta = NULL;
    }

    for (size_t i = 0; i < par.nplanes; ++i) {
        free(fmat[i].f11);
        free(fmat[i].f21);
        free(fmat[i].f12);
        free(fmat[i].f22);
    }
    free(fmat);
    fmat = NULL;

    for (size_t i = 0; i < par.ncomp; ++i) {
        free(par.comp[i]);
    }
    free(par.comp);
    free(par.planes);

    return 0;
}
