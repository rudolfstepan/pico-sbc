#ifndef CALCULATOR_SYMBOLS_H
#define CALCULATOR_SYMBOLS_H

#include <stdbool.h>
#include <stddef.h>

#define CALCULATOR_VARIABLE_COUNT 6
#define CALCULATOR_USER_FUNCTION_COUNT 3
#define CALCULATOR_FAVORITE_COUNT 6
#define CALCULATOR_SYMBOL_EXPRESSION_CAPACITY 96
#define CALCULATOR_FAVORITE_CAPACITY 24

typedef enum {
    CALCULATOR_SYMBOL_OK,
    CALCULATOR_SYMBOL_INVALID_INDEX,
    CALCULATOR_SYMBOL_TOO_LONG,
    CALCULATOR_SYMBOL_RECURSIVE
} calculator_symbol_status_t;

typedef struct {
    double variables[CALCULATOR_VARIABLE_COUNT];
    char functions[CALCULATOR_USER_FUNCTION_COUNT]
                  [CALCULATOR_SYMBOL_EXPRESSION_CAPACITY];
    char favorites[CALCULATOR_FAVORITE_COUNT]
                  [CALCULATOR_FAVORITE_CAPACITY];
} calculator_symbols_t;

void calculator_symbols_init(calculator_symbols_t *symbols);
const char *calculator_variable_name(size_t index);
const char *calculator_function_name(size_t index);
bool calculator_symbols_set_variable(calculator_symbols_t *symbols,
                                     size_t index, double value);
calculator_symbol_status_t calculator_symbols_set_function(
    calculator_symbols_t *symbols, size_t index, const char *expression);
calculator_symbol_status_t calculator_symbols_set_favorite(
    calculator_symbols_t *symbols, size_t index, const char *token);
unsigned int calculator_symbols_function_dependencies(
    const calculator_symbols_t *symbols, size_t index);
bool calculator_symbols_have_cycle(const calculator_symbols_t *symbols);
const char *calculator_symbol_status_text(calculator_symbol_status_t status);

#endif
