#ifndef CALCULATION_STATUS_H
#define CALCULATION_STATUS_H

typedef enum {
    CALC_OK = 0,
    CALC_EMPTY,
    CALC_PARSE_ERROR,
    CALC_DOMAIN_ERROR,
    CALC_RANGE_ERROR,
    CALC_CONVERGENCE_ERROR,
    CALC_OVERFLOW = CALC_RANGE_ERROR
} calc_status_t;

const char *calculation_status_text(calc_status_t status);

#endif
