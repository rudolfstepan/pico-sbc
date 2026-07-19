#include "calculator_engine.h"

#include "tinyexpr.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#define CALC_PI 3.14159265358979323846

static bool use_degrees = true;

struct calc_compiled {
    te_expr *expression;
    double ans;
    double x;
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

calc_status_t calc_engine_evaluate(const char *expression, double ans,
                                   double *result, int *error_position) {
    if (error_position) *error_position = 0;
    if (!expression || !*expression) return CALC_EMPTY;

    const te_variable variables[] = {
        {"ans", &ans, TE_VARIABLE, NULL},
        {"acos", angle_acos, TE_FUNCTION1, NULL},
        {"asin", angle_asin, TE_FUNCTION1, NULL},
        {"atan", angle_atan, TE_FUNCTION1, NULL},
        {"cos", angle_cos, TE_FUNCTION1, NULL},
        {"fac", checked_factorial, TE_FUNCTION1, NULL},
        {"ncr", checked_ncr, TE_FUNCTION2, NULL},
        {"npr", checked_npr, TE_FUNCTION2, NULL},
        {"sin", angle_sin, TE_FUNCTION1, NULL},
        {"tan", angle_tan, TE_FUNCTION1, NULL},
    };

    int error = 0;
    te_expr *compiled = te_compile(expression, variables,
                                   (int)(sizeof variables / sizeof variables[0]),
                                   &error);
    if (!compiled) {
        if (error_position) *error_position = error;
        return CALC_PARSE_ERROR;
    }

    double value = te_eval(compiled);
    te_free(compiled);

    if (isnan(value)) return CALC_DOMAIN_ERROR;
    if (isinf(value)) return CALC_OVERFLOW;
    if (result) *result = value;
    return CALC_OK;
}

const char *calc_engine_status_text(calc_status_t status) {
    switch (status) {
        case CALC_OK: return "OK";
        case CALC_EMPTY: return "ENTER EXPRESSION";
        case CALC_PARSE_ERROR: return "SYNTAX ERROR";
        case CALC_DOMAIN_ERROR: return "MATH ERROR";
        case CALC_OVERFLOW: return "OVERFLOW";
        default: return "ERROR";
    }
}

calc_compiled_t *calc_engine_compile_x(const char *expression, double ans,
                                       int *error_position) {
    if (error_position) *error_position = 0;
    if (!expression || !*expression) return NULL;

    calc_compiled_t *compiled = calloc(1, sizeof *compiled);
    if (!compiled) return NULL;
    compiled->ans = ans;

    const te_variable variables[] = {
        {"ans", &compiled->ans, TE_VARIABLE, NULL},
        {"x", &compiled->x, TE_VARIABLE, NULL},
        {"acos", angle_acos, TE_FUNCTION1, NULL},
        {"asin", angle_asin, TE_FUNCTION1, NULL},
        {"atan", angle_atan, TE_FUNCTION1, NULL},
        {"cos", angle_cos, TE_FUNCTION1, NULL},
        {"fac", checked_factorial, TE_FUNCTION1, NULL},
        {"ncr", checked_ncr, TE_FUNCTION2, NULL},
        {"npr", checked_npr, TE_FUNCTION2, NULL},
        {"sin", angle_sin, TE_FUNCTION1, NULL},
        {"tan", angle_tan, TE_FUNCTION1, NULL},
    };

    int error = 0;
    compiled->expression = te_compile(
        expression, variables, (int)(sizeof variables / sizeof variables[0]),
        &error);
    if (!compiled->expression) {
        if (error_position) *error_position = error;
        free(compiled);
        return NULL;
    }
    return compiled;
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
    free(compiled);
}
