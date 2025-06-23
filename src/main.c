#include <stdio.h>
// #include <errno.h>
// #include <string.h>
#include <stdlib.h>
// #include <limits.h>
// #include <math.h>

#include "strutil.h"
#include "ddscat.h"


int main(void) {

    const char *root = "./sphere";
    char par_file_path[512];
    sprintf(par_file_path, "%s/ddscat.par", root);

    Par par;
    DdscatError ddscat_err;
    // const char* fml_file_name = "w000r000k000.fml";

    ddscat_err = ddscat_parse_par_file(par_file_path, &par);
    if (ddscat_err != DDSCAT_OK) {
        // TODO: report error
        printf("ERROR %d\n", ddscat_err);
        return 1;
    }


    // for (size_t i = 0; i < par.ncomp; ++i) {
        // printf("Material %zu: %s\n", i+1, par.comp[i]);
    // }

    for (size_t i = 0; i < par.nplanes; ++i) {
        printf("Plane %zu: %lf %lf %lf %lf\n", i+1, par.planes[i][0], par.planes[i][1], par.planes[i][2], par.planes[i][3]);
    }


    printf("(%lf, %lf) (%lf, %lf) (%lf, %lf)", par.e01.x.re, par.e01.x.im, par.e01.y.re, par.e01.y.im, par.e01.z.re, par.e01.z.im);



    for (size_t i = 0; i < par.ncomp; ++i) {
        free(par.comp[i]);
    }
    free(par.comp);
    free(par.planes);


    //     {
    //         parse_dielectrics(line, &n_materials, &materials);
            
    //     }
    // }

    // printf("Number of materials: %zu\n", n_materials);
    // for (size_t i = 0; i < n_materials; ++i)
    // {
    //     printf("%s\n", materials[i]);
    // }

    // for (size_t i = 0; i < n_materials; ++i)
    // {
    //     free(materials[i]);
    //     materials[i] = NULL;
    // }
    // free(materials);
    // materials = NULL;
    // fclose(par_file);
    return 0;
}
