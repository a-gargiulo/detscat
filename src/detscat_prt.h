#ifndef DETSCAT_PRT_H
#define DETSCAT_PRT_H

#include <stdbool.h>
#include <stddef.h>

#include "detscat.h"
#include "detscat_math.h"
#include "detscat_str.h"

typedef struct {
    int w;  // W from DDSCAT file name wxxxryyykzzz.*
    int r;  // R from DDSCAT file name wxxxryyykzzz.*
    int k;  // K from DDSCAT file name wxxxryyykzzz.*
} DetScatPrtCaseId;

typedef struct {
    Vec3 a1;
    Vec3 a2;
    double theta;
    double phi;
    double beta;
} DetScatPrtOrientation;

typedef struct {
    Str type_id;
    double wavelength_nm;
    double eff_radius_um;
    DetScatPrtOrientation orientation;
    Vec3 position;
    DetScatPrtCaseId case_id;
} DetScatPrtParticle;

typedef struct {
    Str type_id;
    Str data_dir;
} DetScatPrtType;

typedef struct DetScatPrt {
    DetScatPrtType *types;
    DetScatPrtParticle *particles;
    size_t n_types;
    size_t n_particles;
} DetScatPrt;

void detscat_prt_free(DetScatPrt *prt);
bool detscat_prt_load(const char *prt_file_path, DetScatPrt *prt, DetScatError *err);
void detscat_prt_free_subset(DetScatPrt *prt, size_t types_count,
                             size_t particles_count);
void detscat_prt_transform(DetScatPrtParticle *particles, size_t n, const Vec3 *origin, const Mat3 *rotmat);
void detscat_prt_orientation_vec_to_angles(DetScatPrt *prt);

#endif  // DETSCAT_PRT_H
