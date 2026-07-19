#ifndef CALCULATOR_UNITS_H
#define CALCULATOR_UNITS_H

#include "unit_engine.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    UNITS_VIEW_CONVERTER,
    UNITS_VIEW_CONSTANTS
} calculator_units_view_t;

typedef enum {
    UNITS_OUTPUT_NONE,
    UNITS_OUTPUT_ANS,
    UNITS_OUTPUT_EDITOR
} calculator_units_output_t;

typedef struct {
    unit_category_t category;
    size_t from_index;
    size_t to_index;
    double input;
    double result;
    bool has_input;
    bool has_result;
    calculator_units_view_t view;
    size_t constant_index;
} calculator_units_t;

void calculator_units_init(calculator_units_t *units);
calculator_units_output_t calculator_units_activate(
    calculator_units_t *units, const char *token, double ans,
    double *output, char *message, size_t message_size);

#endif
