#ifndef HIGH_PRECISION_ENGINE_H
#define HIGH_PRECISION_ENGINE_H

#include "calculator_symbols.h"
#include "decimal_engine.h"

#include <stdbool.h>

#define HIGH_PRECISION_DECIMAL_DIGITS 80u

typedef enum {
    HIGH_PRECISION_STATUS_OK,
    HIGH_PRECISION_STATUS_UNSUPPORTED,
    HIGH_PRECISION_STATUS_SYNTAX,
    HIGH_PRECISION_STATUS_DIV_ZERO,
    HIGH_PRECISION_STATUS_DOMAIN,
    HIGH_PRECISION_STATUS_RANGE,
    HIGH_PRECISION_STATUS_MEMORY
} high_precision_status_t;

typedef struct {
    char text[DECIMAL_ENGINE_TEXT_CAPACITY];
    double approximation;
} high_precision_result_t;

high_precision_status_t high_precision_engine_evaluate(
    const char *expression, const char *ans_text,
    const calculator_symbols_t *symbols, bool degrees,
    high_precision_result_t *result, int *error_position);
const char *high_precision_status_text(high_precision_status_t status);

#endif
