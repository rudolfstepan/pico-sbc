#include "calculator_engine.h"

#include <math.h>
#include <stdio.h>

static int failures;

static void expect_value(const char *expression, double expected, double tolerance) {
    double result = 0.0;
    int error = 0;
    calc_status_t status = calc_engine_evaluate(expression, 42.0, &result, &error);
    if (status != CALC_OK || fabs(result - expected) > tolerance) {
        printf("FAIL: %s -> status=%d result=%.12g error=%d\n",
               expression, status, result, error);
        failures++;
    }
}

static void expect_status(const char *expression, calc_status_t expected) {
    double result = 0.0;
    int error = 0;
    calc_status_t status = calc_engine_evaluate(expression, 0.0, &result, &error);
    if (status != expected) {
        printf("FAIL: %s -> status=%d expected=%d error=%d\n",
               expression, status, expected, error);
        failures++;
    }
}

int main(void) {
    calc_engine_set_degrees(true);
    expect_value("2+3*4", 14.0, 1e-12);
    expect_value("sqrt(81)", 9.0, 1e-12);
    expect_value("10%3", 1.0, 1e-12);
    expect_value("2^3^2", 512.0, 1e-12);
    expect_value("sin(30)", 0.5, 1e-12);
    expect_value("asin(0.5)", 30.0, 1e-10);
    expect_value("fac(5)", 120.0, 1e-12);
    expect_value("ncr(6,2)", 15.0, 1e-12);
    expect_value("npr(6,2)", 30.0, 1e-12);
    expect_value("ans+8", 50.0, 1e-12);
    expect_status("fac(2.5)", CALC_DOMAIN_ERROR);
    expect_status("1/0", CALC_OVERFLOW);
    expect_status("2+(", CALC_PARSE_ERROR);

    calc_engine_set_degrees(false);
    expect_value("sin(pi/2)", 1.0, 1e-12);

    int graph_error = 0;
    calc_compiled_t *graph = calc_engine_compile_x("x^2+ans", 2.0,
                                                    &graph_error);
    double graph_result = 0.0;
    if (!graph || !calc_engine_evaluate_x(graph, 3.0, &graph_result) ||
        fabs(graph_result - 11.0) > 1e-12) {
        printf("FAIL: compiled x expression, error=%d\n", graph_error);
        failures++;
    }
    calc_engine_free(graph);

    if (failures) {
        printf("%d calculator engine test(s) failed\n", failures);
        return 1;
    }
    printf("calculator engine tests passed\n");
    return 0;
}
