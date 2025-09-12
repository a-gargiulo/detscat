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
        !detscat_str_reserve(&cfg->particles_file_path,
                             DETSCAT_CFG_PATH_INIT)) {
        detscat_str_free(&cfg->particles_file_path);
        DETSCAT_SET_ERROR(err, DETSCAT_ERR_MEMORY,
                          "Failed to allocate memory for cfg data");
        return false;
    }

    return true;
}

void detscat_cfg_destroy(DetScatConfig *cfg) {
    if (!cfg) return;
    detscat_str_free(&cfg->particles_file_path);
}
