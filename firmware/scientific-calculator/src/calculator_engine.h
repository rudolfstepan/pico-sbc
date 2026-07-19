#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct calc_compiled calc_compiled_t;

typedef enum {
    CALC_OK = 0,
    CALC_EMPTY,
    CALC_PARSE_ERROR,
    CALC_DOMAIN_ERROR,
    CALC_OVERFLOW
} calc_status_t;

void calc_engine_set_degrees(bool degrees);
bool calc_engine_uses_degrees(void);
calc_status_t calc_engine_evaluate(const char *expression, double ans,
                                   double *result, int *error_position);
const char *calc_engine_status_text(calc_status_t status);
calc_compiled_t *calc_engine_compile_x(const char *expression, double ans,
                                       int *error_position);
bool calc_engine_evaluate_x(calc_compiled_t *compiled, double x, double *result);
void calc_engine_free(calc_compiled_t *compiled);
