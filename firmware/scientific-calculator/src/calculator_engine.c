#include "calculator_engine.h"

#include "tinyexpr.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define CALC_PI 3.14159265358979323846

static bool use_degrees = true;

struct calc_compiled;

typedef struct {
    struct calc_compiled *owner;
    size_t index;
    double x;
} user_function_context_t;

struct calc_compiled {
    te_expr *expression;
    te_expr *user_expressions[CALCULATOR_USER_FUNCTION_COUNT];
    calculator_symbols_t symbols;
    user_function_context_t user_contexts[CALCULATOR_USER_FUNCTION_COUNT];
    double ans;
    double x;
    unsigned int active_functions;
};

static double to_radians(double value) {
    return use_degrees ? value * CALC_PI / 180.0 : value;
}

static double from_radians(double value) {
    return use_degrees ? value * 180.0 / CALC_PI : value;
}

static double angle_sin(double value) { return sin(to_radians(value)); }
static double angle_cos(double value) { return cos(to_radians(value)); }
static double angle_tan(double value) { return tan(to_radians(value)); }
static double angle_asin(double value) { return from_radians(asin(value)); }
static double angle_acos(double value) { return from_radians(acos(value)); }
static double angle_atan(double value) { return from_radians(atan(value)); }

static bool is_nonnegative_integer(double value) {
    return isfinite(value) && value >= 0.0 && value == floor(value);
}

static double checked_factorial(double value) {
    if (!is_nonnegative_integer(value)) return NAN;
    if (value > 170.0) return INFINITY;

    double result = 1.0;
    for (unsigned int i = 2; i <= (unsigned int)value; ++i) {
        result *= i;
    }
    return result;
}

static double checked_ncr(double n, double r) {
    if (!is_nonnegative_integer(n) || !is_nonnegative_integer(r) || r > n) {
        return NAN;
    }

    unsigned int nn = (unsigned int)n;
    unsigned int rr = (unsigned int)r;
    if (rr > nn - rr) rr = nn - rr;

    double result = 1.0;
    for (unsigned int i = 1; i <= rr; ++i) {
        result = result * (double)(nn - rr + i) / (double)i;
        if (!isfinite(result)) return INFINITY;
    }
    return result;
}

static double checked_npr(double n, double r) {
    if (!is_nonnegative_integer(n) || !is_nonnegative_integer(r) || r > n) {
        return NAN;
    }

    double result = 1.0;
    for (unsigned int i = 0; i < (unsigned int)r; ++i) {
        result *= n - (double)i;
        if (!isfinite(result)) return INFINITY;
    }
    return result;
}

void calc_engine_set_degrees(bool degrees) {
    use_degrees = degrees;
}

bool calc_engine_uses_degrees(void) {
    return use_degrees;
}

static double evaluate_user_function(void *context, double argument) {
    user_function_context_t *function = context;
    calc_compiled_t *compiled = function->owner;
    unsigned int bit = 1u << function->index;
    if (!compiled->user_expressions[function->index] ||
        (compiled->active_functions & bit)) {
        return NAN;
    }

    double previous_x = function->x;
    function->x = argument;
    compiled->active_functions |= bit;
    double result = te_eval(compiled->user_expressions[function->index]);
    compiled->active_functions &= ~bit;
    function->x = previous_x;
    return result;
}

static size_t build_variables(calc_compiled_t *compiled, double *x,
                              te_variable *variables) {
    size_t count = 0;
    static const char *variable_names[CALCULATOR_VARIABLE_COUNT] = {
        "A", "B", "C", "D", "E", "F"
    };
    static const char *function_names[CALCULATOR_USER_FUNCTION_COUNT] = {
        "f1", "f2", "f3"
    };

    for (size_t i = 0; i < CALCULATOR_VARIABLE_COUNT; ++i) {
        variables[count++] = (te_variable){
            variable_names[i], &compiled->symbols.variables[i],
            TE_VARIABLE, NULL
        };
    }
    variables[count++] = (te_variable){
        "ans", &compiled->ans, TE_VARIABLE, NULL
    };
    if (x) {
        variables[count++] = (te_variable){"x", x, TE_VARIABLE, NULL};
    }
    for (size_t i = 0; i < CALCULATOR_USER_FUNCTION_COUNT; ++i) {
        variables[count++] = (te_variable){
            function_names[i], evaluate_user_function, TE_CLOSURE1,
            &compiled->user_contexts[i]
        };
    }

    variables[count++] = (te_variable){"acos", angle_acos, TE_FUNCTION1, NULL};
    variables[count++] = (te_variable){"asin", angle_asin, TE_FUNCTION1, NULL};
    variables[count++] = (te_variable){"atan", angle_atan, TE_FUNCTION1, NULL};
    variables[count++] = (te_variable){"cos", angle_cos, TE_FUNCTION1, NULL};
    variables[count++] = (te_variable){"fac", checked_factorial, TE_FUNCTION1, NULL};
    variables[count++] = (te_variable){"ncr", checked_ncr, TE_FUNCTION2, NULL};
    variables[count++] = (te_variable){"npr", checked_npr, TE_FUNCTION2, NULL};
    variables[count++] = (te_variable){"sin", angle_sin, TE_FUNCTION1, NULL};
    variables[count++] = (te_variable){"tan", angle_tan, TE_FUNCTION1, NULL};
    return count;
}

static bool dependencies_are_defined(const calculator_symbols_t *symbols,
                                     size_t function) {
    unsigned int dependencies =
        calculator_symbols_function_dependencies(symbols, function);
    for (size_t i = 0; i < CALCULATOR_USER_FUNCTION_COUNT; ++i) {
        if ((dependencies & (1u << i)) && !symbols->functions[i][0]) {
            return false;
        }
    }
    return true;
}

static calc_compiled_t *compile_expression(
    const char *expression, double ans, const calculator_symbols_t *symbols,
    bool include_x, size_t *invalid_function, int *error_position) {
    if (error_position) *error_position = 0;
    if (invalid_function) *invalid_function = CALCULATOR_USER_FUNCTION_COUNT;

    calc_compiled_t *compiled = calloc(1, sizeof *compiled);
    if (!compiled) return NULL;
    compiled->ans = ans;
    if (symbols) {
        memcpy(&compiled->symbols, symbols, sizeof compiled->symbols);
    } else {
        calculator_symbols_init(&compiled->symbols);
    }
    for (size_t i = 0; i < CALCULATOR_USER_FUNCTION_COUNT; ++i) {
        compiled->user_contexts[i].owner = compiled;
        compiled->user_contexts[i].index = i;
    }

    te_variable variables[21];
    for (size_t i = 0; i < CALCULATOR_USER_FUNCTION_COUNT; ++i) {
        if (!compiled->symbols.functions[i][0]) continue;
        if (!dependencies_are_defined(&compiled->symbols, i)) {
            if (invalid_function) *invalid_function = i;
            calc_engine_free(compiled);
            return NULL;
        }
        size_t variable_count = build_variables(
            compiled, &compiled->user_contexts[i].x, variables);
        int error = 0;
        compiled->user_expressions[i] = te_compile(
            compiled->symbols.functions[i], variables, (int)variable_count,
            &error);
        if (!compiled->user_expressions[i]) {
            if (invalid_function) *invalid_function = i;
            if (error_position) *error_position = error;
            calc_engine_free(compiled);
            return NULL;
        }
    }

    size_t variable_count = build_variables(
        compiled, include_x ? &compiled->x : NULL, variables);
    int error = 0;
    compiled->expression = te_compile(expression, variables,
                                      (int)variable_count, &error);
    if (!compiled->expression) {
        if (error_position) *error_position = error;
        calc_engine_free(compiled);
        return NULL;
    }
    return compiled;
}

calc_status_t calc_engine_evaluate_symbols(
    const char *expression, double ans, const calculator_symbols_t *symbols,
    double *result, int *error_position) {
    if (error_position) *error_position = 0;
    if (!expression || !*expression) return CALC_EMPTY;

    calc_compiled_t *compiled = compile_expression(
        expression, ans, symbols, false, NULL, error_position);
    if (!compiled) return CALC_PARSE_ERROR;

    double value = te_eval(compiled->expression);
    calc_engine_free(compiled);

    if (isnan(value)) return CALC_DOMAIN_ERROR;
    if (isinf(value)) return CALC_OVERFLOW;
    if (result) *result = value;
    return CALC_OK;
}

calc_status_t calc_engine_evaluate(const char *expression, double ans,
                                   double *result, int *error_position) {
    return calc_engine_evaluate_symbols(expression, ans, NULL, result,
                                        error_position);
}

const char *calc_engine_status_text(calc_status_t status) {
    return calculation_status_text(status);
}

calc_compiled_t *calc_engine_compile_x_symbols(
    const char *expression, double ans, const calculator_symbols_t *symbols,
    int *error_position) {
    if (error_position) *error_position = 0;
    if (!expression || !*expression) return NULL;
    return compile_expression(expression, ans, symbols, true, NULL,
                              error_position);
}

calc_compiled_t *calc_engine_compile_x(const char *expression, double ans,
                                       int *error_position) {
    return calc_engine_compile_x_symbols(expression, ans, NULL,
                                         error_position);
}

calc_status_t calc_engine_validate_symbols(
    const calculator_symbols_t *symbols, size_t *invalid_function,
    int *error_position) {
    if (!symbols || calculator_symbols_have_cycle(symbols)) {
        if (invalid_function) {
            *invalid_function = CALCULATOR_USER_FUNCTION_COUNT;
        }
        if (error_position) *error_position = 0;
        return CALC_PARSE_ERROR;
    }
    calc_compiled_t *compiled = compile_expression(
        "0", 0.0, symbols, false, invalid_function, error_position);
    if (!compiled) return CALC_PARSE_ERROR;
    calc_engine_free(compiled);
    return CALC_OK;
}

bool calc_engine_evaluate_x(calc_compiled_t *compiled, double x, double *result) {
    if (!compiled || !compiled->expression) return false;
    compiled->x = x;
    double value = te_eval(compiled->expression);
    if (!isfinite(value)) return false;
    if (result) *result = value;
    return true;
}

void calc_engine_free(calc_compiled_t *compiled) {
    if (!compiled) return;
    te_free(compiled->expression);
    for (size_t i = 0; i < CALCULATOR_USER_FUNCTION_COUNT; ++i) {
        te_free(compiled->user_expressions[i]);
    }
    free(compiled);
}
