#include "detscat_prt.h"

#include "detscat.h"

#include "detscat_error.h"
#include "detscat_str.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


// --- Internal helpers (PRIVATE) ---
static void detscat_prt_clear_types(DetScatPrt *prt, size_t count) {
    if (!prt || !prt->types) return;

    for (size_t i = 0; i < count; ++i) {
        detscat_str_free(&prt->types[i].type_id);
        detscat_str_free(&prt->types[i].data_dir);
    }

    free(prt->types);
    prt->types = NULL;
}

static void detscat_prt_clear_particles(DetScatPrt *prt, size_t count) {
    if (!prt || !prt->particles) return;

    for (size_t i = 0; i < count; ++i) {
        detscat_str_free(&prt->particles[i].type_id);

        memset(&prt->particles[i].position, 0,
               sizeof(prt->particles[i].position));

        memset(&prt->particles[i].case_id, 0,
               sizeof(prt->particles[i].case_id));
    }

    free(prt->particles);
    prt->particles = NULL;
}

// --- Internal helpers (SHARED) ---
void detscat_prt_free(DetScatPrt *prt) {
    if (!prt) return;

    detscat_prt_clear_types(prt, prt->n_types);
    detscat_prt_clear_particles(prt, prt->n_particles);

    prt->n_types = 0;
    prt->n_particles = 0;
}

void detscat_prt_free_subset(DetScatPrt *prt, size_t types_count,
                             size_t particles_count) {
    if (!prt) return;

    detscat_prt_clear_types(prt, types_count);
    detscat_prt_clear_particles(prt, particles_count);

    prt->n_types = 0;
    prt->n_particles = 0;
}
