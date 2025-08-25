#ifndef DETSCAT_H
#define DETSCAT_H

#include "detscat_diag.h"
#ifdef DETSCAT_ENABLE_LOGGING
#  include "detscat_log.h"
#endif


int detscat_run(int argc, char **argv, DetScatDiagnose *diag);
// double detscat_calculate_incident_field_strength(double d, double E_p_mj, double tau_p_ns);

#endif  //  DETSCAT_H
