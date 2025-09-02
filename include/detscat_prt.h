#ifndef DETSCAT_PARTICLES_H
#define DETSCAT_PARTICLES_H

#include "detscat_diag.h"

#include "detscat_math.h"
#include "detscat_str.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef struct DetScatDdscatCaseId DetScatDdscatCaseId;

typedef struct {
    Vec3                    position;
    Str                     type_id;
    DetScatDdscatCaseId     *case_id;
} DetScatPrtParticle;

typedef struct {
    Str                     type_id;
    Str                     data_dir;
} DetScatPrtType;

typedef struct DetScatPrt {
    DetScatPrtType         *types;
    DetScatPrtParticle     *particles;
    size_t                  n_types;
    size_t                  n_particles;
} DetScatPrt;

bool detscat_prt_create(DetScatPrt **prt, DetScatDiagnose *diag);

void detscat_prt_destroy(DetScatPrt **prt);

bool detscat_prt_load(const char *file_path, DetScatPrt *prt, DetScatDiagnose *diag);

void detscat_prt_free_subset(DetScatPrt *prt, size_t types_count, size_t particles_count); 


#endif  // DETSCAT_PARTICLES_H
