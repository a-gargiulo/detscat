#include "detscat.h"

#include <math.h>

#include "detscat_config.h"
#include "detscat_const.h"
#include "mymath.h"

#define DETSCAT_ERR_MSG_MAX 256 

typedef enum {
    DETSCAT_OK = 0,
    DETSCAT_ERR_COMMAND_LINE_ARGS
} DetScatStatus;


typedef struct {
    

    DetScatStatus stauts;
    char err_msg[DETSCAT_ERR_MSG];
    

} DetScat;



int detscat_run(int argc, char **argv) {

}




double detscat_calculate_incident_field_strength(double d, double E_p_mj, double tau_p_ns) {
    double A = d * d * DETSCAT_CONST_MM2M * DETSCAT_CONST_MM2M * M_PI / 4.0;
    double I = E_p_mj * DETSCAT_CONST_MJ2J / tau_p_ns / DETSCAT_CONST_NS2S / A;
    return sqrt(2 * I / DETSCAT_CONST_C_MS / DETSCAT_CONST_EPS0_F_M);
}


