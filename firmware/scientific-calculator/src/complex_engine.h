#ifndef COMPLEX_ENGINE_H
#define COMPLEX_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    double real;
    double imag;
} complex_value_t;

typedef enum {
    COMPLEX_STATUS_OK,
    COMPLEX_STATUS_EMPTY,
    COMPLEX_STATUS_SYNTAX,
    COMPLEX_STATUS_DIV_ZERO,
    COMPLEX_STATUS_DOMAIN,
    COMPLEX_STATUS_RANGE
} complex_status_t;

complex_status_t complex_engine_evaluate(const char *expression,
                                         bool degrees,
                                         complex_value_t *result,
                                         int *error_position);
bool complex_engine_format(complex_value_t value, bool polar, bool degrees,
                           char *output, size_t output_size);
const char *complex_engine_status_text(complex_status_t status);

#endif
