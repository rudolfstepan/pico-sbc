#ifndef CALCULATOR_COMPLEX_H
#define CALCULATOR_COMPLEX_H

#include "complex_engine.h"
#include "expression_editor.h"

#include <stdbool.h>
#include <stddef.h>

#define CALCULATOR_COMPLEX_HISTORY_CAPACITY 8u

typedef struct {
    char expression[EXPRESSION_EDITOR_CAPACITY];
    complex_value_t result;
} calculator_complex_history_entry_t;

typedef struct {
    expression_editor_t editor;
    complex_value_t result;
    bool has_result;
    bool polar_view;
    bool history_view;
    bool just_evaluated;
    calculator_complex_history_entry_t
        history[CALCULATOR_COMPLEX_HISTORY_CAPACITY];
    size_t history_count;
    size_t history_index;
} calculator_complex_t;

void calculator_complex_init(calculator_complex_t *complex);
void calculator_complex_activate(calculator_complex_t *complex,
                                 const char *token, bool degrees,
                                 char *message, size_t message_size);

#endif
