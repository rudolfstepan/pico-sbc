#pragma once

#include "calculation_status.h"
#include "calculator_symbols.h"
#include "decimal_engine.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct calc_compiled calc_compiled_t;

#define CALCULATOR_RESULT_TEXT_CAPACITY DECIMAL_ENGINE_TEXT_CAPACITY

typedef struct {
    double value;
    char text[CALCULATOR_RESULT_TEXT_CAPACITY];
    bool decimal;
    bool exact;
} calculator_result_t;

void calc_engine_set_degrees(bool degrees);
bool calc_engine_uses_degrees(void);
calc_status_t calc_engine_evaluate(const char *expression, double ans,
                                   double *result, int *error_position);
calc_status_t calc_engine_evaluate_symbols(
    const char *expression, double ans, const calculator_symbols_t *symbols,
    double *result, int *error_position);
calc_status_t calc_engine_evaluate_precise_symbols(
    const char *expression, double ans, const char *ans_text,
    const calculator_symbols_t *symbols, calculator_result_t *result,
    int *error_position);
const char *calc_engine_status_text(calc_status_t status);
calc_compiled_t *calc_engine_compile_x(const char *expression, double ans,
                                       int *error_position);
calc_compiled_t *calc_engine_compile_x_symbols(
    const char *expression, double ans, const calculator_symbols_t *symbols,
    int *error_position);
calc_status_t calc_engine_validate_symbols(
    const calculator_symbols_t *symbols, size_t *invalid_function,
    int *error_position);
bool calc_engine_evaluate_x(calc_compiled_t *compiled, double x, double *result);
void calc_engine_free(calc_compiled_t *compiled);
