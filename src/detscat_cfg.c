#include "detscat_cfg.h"

#include "detscat.h"

#include "detscat_error.h"
#include "detscat_limits.h"
#include "detscat_str.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// --- Internal helpers (SHARED) ---
bool detscat_cfg_init(DetScatConfig *cfg, DetScatError *err) {
    assert(cfg);

    if (!detscat_str_init(&cfg->particles_file_path) ||
        !detscat_str_reserve(&cfg->particles_file_path, DETSCAT_CFG_PATH_INIT))
        goto error_cleanup;
    
    if (!detscat_str_init(&cfg->polarization_type) ||
        !detscat_str_reserve(&cfg->polarization_type, DETSCAT_CFG_STRVAR_INIT)) 
        goto error_cleanup;

    if (!detscat_str_init(&cfg->polarization_axis) ||
        !detscat_str_reserve(&cfg->polarization_axis, DETSCAT_CFG_STRVAR_INIT))
        goto error_cleanup;

    return true;

error_cleanup:
    detscat_cfg_free(cfg);
    DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                      "Failed to allocate memory for cfg data");
    return false;
}

void detscat_cfg_free(DetScatConfig *cfg) {
    if (!cfg) return;
    detscat_str_free(&cfg->particles_file_path);
    detscat_str_free(&cfg->polarization_type);
    detscat_str_free(&cfg->polarization_axis);
}
