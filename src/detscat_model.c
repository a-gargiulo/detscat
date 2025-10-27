#include "detscat_model.h"

#include "detscat_const.h"

#include <math.h>


double detscat_model_calculate_field_strength(double D_beam_mm,
                                              double E_pulse_mj,
                                              double tau_pulse_ns) {
    double c = DETSCAT_CONST_C_M_S;
    double eps0 = DETSCAT_CONST_EPS0_F_M;
    double D_beam_m = (D_beam_mm * DETSCAT_CONST_MM2M);
    double A = D_beam_m * D_beam_m * M_PI / 4.0;
    double E_p = E_pulse_mj * DETSCAT_CONST_MJ2J;
    double tau_p = tau_pulse_ns * DETSCAT_CONST_NS2S;

    return sqrt(2 * E_p / (tau_p * A * c * eps0));
}

