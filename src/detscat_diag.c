#include "detscat_diag.h"

const char *detscat_diag_status_to_str(DetScatStatus status) {
    switch (status) {
        case DETSCAT_OK:                  return "OK";
        case DETSCAT_ERR_MISSING_CMD_ARG: return "Missing command argument";
        case DETSCAT_ERR_PARSING:         return "Parsing error";
        case DETSCAT_ERR_ALLOC:           return "Memory allocation error";
        case DETSCAT_ERR_LOOKUP:          return "Lookup error";
        case DETSCAT_ERR_OVERFLOW:        return "Overflow error";
        case DETSCAT_ERR_INVALID_ARG:     return "Invalid argument";
        case DETSCAT_ERR_ARG_RANGE:       return "Argument out of range";
        default:                          return "Unknown error";
    }
}
