#include "calculator_symbols.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *variable_names[CALCULATOR_VARIABLE_COUNT] = {
    "A", "B", "C", "D", "E", "F"
};

static const char *function_names[CALCULATOR_USER_FUNCTION_COUNT] = {
    "F1", "F2", "F3"
};

static const char *default_favorites[CALCULATOR_FAVORITE_COUNT] = {
    "f1(", "f2(", "f3(", "sin(", "cos(", "sqrt("
};

void calculator_symbols_init(calculator_symbols_t *symbols) {
    memset(symbols, 0, sizeof *symbols);
    for (size_t i = 0; i < CALCULATOR_VARIABLE_COUNT; ++i) {
        snprintf(symbols->variable_text[i],
                 sizeof symbols->variable_text[i], "0");
    }
    for (size_t i = 0; i < CALCULATOR_FAVORITE_COUNT; ++i) {
        snprintf(symbols->favorites[i], sizeof symbols->favorites[i],
                 "%s", default_favorites[i]);
    }
}

const char *calculator_variable_name(size_t index) {
    return index < CALCULATOR_VARIABLE_COUNT ? variable_names[index] : "?";
}

const char *calculator_function_name(size_t index) {
    return index < CALCULATOR_USER_FUNCTION_COUNT ? function_names[index] : "?";
}

bool calculator_symbols_set_variable(calculator_symbols_t *symbols,
                                     size_t index, double value) {
    if (index >= CALCULATOR_VARIABLE_COUNT || !isfinite(value)) return false;
    symbols->variables[index] = value;
    snprintf(symbols->variable_text[index],
             sizeof symbols->variable_text[index], "%.17g", value);
    return true;
}

bool calculator_symbols_set_variable_precise(calculator_symbols_t *symbols,
                                             size_t index, double value,
                                             const char *text) {
    if (index >= CALCULATOR_VARIABLE_COUNT || !isfinite(value) || !text ||
        !*text || strlen(text) >= sizeof symbols->variable_text[index]) {
        return false;
    }
    char *end = NULL;
    double parsed = strtod(text, &end);
    if (!end || *end || !isfinite(parsed)) return false;
    symbols->variables[index] = value;
    snprintf(symbols->variable_text[index],
             sizeof symbols->variable_text[index], "%s", text);
    return true;
}

static bool identifier_matches_function(const char *start, size_t length,
                                        size_t function) {
    return length == 2 && tolower((unsigned char)start[0]) == 'f' &&
           start[1] == (char)('1' + function);
}

static unsigned int expression_dependencies(const char *expression) {
    unsigned int dependencies = 0;
    const char *cursor = expression;
    while (*cursor) {
        if (isalpha((unsigned char)*cursor) || *cursor == '_') {
            const char *start = cursor++;
            while (isalnum((unsigned char)*cursor) || *cursor == '_') cursor++;
            size_t length = (size_t)(cursor - start);
            for (size_t function = 0;
                 function < CALCULATOR_USER_FUNCTION_COUNT; ++function) {
                if (identifier_matches_function(start, length, function)) {
                    dependencies |= 1u << function;
                }
            }
        } else {
            cursor++;
        }
    }
    return dependencies;
}

unsigned int calculator_symbols_function_dependencies(
    const calculator_symbols_t *symbols, size_t index) {
    if (index >= CALCULATOR_USER_FUNCTION_COUNT) return 0;
    return expression_dependencies(symbols->functions[index]);
}

static bool dependency_cycle(const calculator_symbols_t *symbols,
                             size_t function, unsigned int visiting,
                             unsigned int visited) {
    unsigned int bit = 1u << function;
    if (visiting & bit) return true;
    if (visited & bit) return false;
    visiting |= bit;
    unsigned int dependencies =
        calculator_symbols_function_dependencies(symbols, function);
    for (size_t dependency = 0;
         dependency < CALCULATOR_USER_FUNCTION_COUNT; ++dependency) {
        if ((dependencies & (1u << dependency)) &&
            dependency_cycle(symbols, dependency, visiting, visited | bit)) {
            return true;
        }
    }
    return false;
}

bool calculator_symbols_have_cycle(const calculator_symbols_t *symbols) {
    for (size_t function = 0;
         function < CALCULATOR_USER_FUNCTION_COUNT; ++function) {
        if (dependency_cycle(symbols, function, 0, 0)) return true;
    }
    return false;
}

calculator_symbol_status_t calculator_symbols_set_function(
    calculator_symbols_t *symbols, size_t index, const char *expression) {
    if (index >= CALCULATOR_USER_FUNCTION_COUNT || !expression) {
        return CALCULATOR_SYMBOL_INVALID_INDEX;
    }
    if (strlen(expression) >= sizeof symbols->functions[index]) {
        return CALCULATOR_SYMBOL_TOO_LONG;
    }

    char previous[CALCULATOR_SYMBOL_EXPRESSION_CAPACITY];
    snprintf(previous, sizeof previous, "%s", symbols->functions[index]);
    snprintf(symbols->functions[index], sizeof symbols->functions[index],
             "%s", expression);
    if (calculator_symbols_have_cycle(symbols)) {
        snprintf(symbols->functions[index], sizeof symbols->functions[index],
                 "%s", previous);
        return CALCULATOR_SYMBOL_RECURSIVE;
    }
    return CALCULATOR_SYMBOL_OK;
}

calculator_symbol_status_t calculator_symbols_set_favorite(
    calculator_symbols_t *symbols, size_t index, const char *token) {
    if (index >= CALCULATOR_FAVORITE_COUNT || !token) {
        return CALCULATOR_SYMBOL_INVALID_INDEX;
    }
    if (strlen(token) >= sizeof symbols->favorites[index]) {
        return CALCULATOR_SYMBOL_TOO_LONG;
    }
    snprintf(symbols->favorites[index], sizeof symbols->favorites[index],
             "%s", token);
    return CALCULATOR_SYMBOL_OK;
}

const char *calculator_symbol_status_text(calculator_symbol_status_t status) {
    switch (status) {
        case CALCULATOR_SYMBOL_OK: return "OK";
        case CALCULATOR_SYMBOL_INVALID_INDEX: return "INVALID SYMBOL";
        case CALCULATOR_SYMBOL_TOO_LONG: return "DEFINITION TOO LONG";
        case CALCULATOR_SYMBOL_RECURSIVE: return "RECURSION BLOCKED";
        default: return "SYMBOL ERROR";
    }
}
