#ifndef DETSCAT_H
#define DETSCAT_H

#include "detscat_diag.h"
#include "detscat_cfg.h"
#include "detscat_prt.h"
#ifdef DETSCAT_ENABLE_LOGGING
#include "detscat_log.h" 
#endif


#include <stdio.h>
#include <stdbool.h>

// #include "detscat_diagnose.h"
// #ifdef DETSCAT_ENABLE_LOGGING
// #include "detscat_log.h"
// #endif

// detscat_load_cfg()
// DetScatStatus detscat_run();

typedef struct {
    DetScatConfig *cfg;
    DetScatPrt *prt;
} DetScat;


bool detscat_init(void);
bool detscat_terminate(void);
bool detscat_load_data(const char *config_path, DetScat* detscat, DetScatDiagnose* diag);

void detscat_data_free(DetScat *detscat);

void detscat_print_cfg(const DetScatConfig *cfg);

#endif  //  DETSCAT_H
