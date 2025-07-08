#include "detscat.h"

#include <math.h>

#include "config.h"
#include "mymath.h"

#define C_MS 299792458
#define EPS0_F_M 8.8541878188e-12

double detscat_calculate_incident_field_strength(double d, double E_p_mj, double tau_p_ns) {
    double A = d * d * MM2M * MM2M * M_PI / 4.0;
    double I = E_p_mj * MJ2J / tau_p_ns / NS2S / A;
    return sqrt(2 * I / C_MS / EPS0_F_M);
}


