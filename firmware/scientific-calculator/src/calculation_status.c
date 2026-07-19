#include "calculation_status.h"

const char *calculation_status_text(calc_status_t status) {
    switch (status) {
        case CALC_OK: return "OK";
        case CALC_EMPTY: return "ENTER EXPRESSION";
        case CALC_PARSE_ERROR: return "SYNTAX ERROR";
        case CALC_DOMAIN_ERROR: return "MATH ERROR";
        case CALC_RANGE_ERROR: return "RANGE ERROR";
        case CALC_CONVERGENCE_ERROR: return "NO CONVERGENCE";
        default: return "ERROR";
    }
}
