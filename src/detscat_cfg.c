#include "detscat_cfg.h"

#include "detscat.h"

#include "detscat_error.h"
#include "detscat_limits.h"
#include "detscat_str.h"

#include <stddef.h>
#include <stdlib.h>

// --- Internal helpers (SHARED) ---
DetScatConfig *detscat_cfg_create(DetScatError *err) {
    DetScatConfig *cfg = calloc(1, sizeof(*cfg));
    if (!cfg) goto cleanup;

    if (!detscat_str_init(&cfg->particles_file_path) ||
        !detscat_str_reserve(&cfg->particles_file_path,
                             DETSCAT_CFG_PATH_INIT)) {
        detscat_str_free(&cfg->particles_file_path);
        goto cleanup;
    }

    return cfg;

cleanup:
    free(cfg);
    DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                     "Failed to allocate memory for cfg data");
    return NULL;
}

void detscat_cfg_destroy(DetScatConfig **cfg) {
    if (!cfg || !*cfg) return;
    detscat_str_free(&(*cfg)->particles_file_path);
    free(*cfg);
    *cfg = NULL;
}
