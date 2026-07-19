#include "numerical_analysis.h"

#include <math.h>
#include <stdio.h>

#define TEST_PI 3.14159265358979323846

static int failures;

static bool quadratic(void *context, double x, double *y) {
    (void)context;
    *y = x * x - 2.0;
    return true;
}

static bool cosine_fixed_point(void *context, double x, double *y) {
    (void)context;
    *y = cos(x) - x;
    return true;
}

static bool sine_function(void *context, double x, double *y) {
    (void)context;
    *y = sin(x);
    return true;
}

static bool line_function(void *context, double x, double *y) {
    (void)context;
    *y = 2.0 - x;
    return true;
}

static bool positive_quadratic(void *context, double x, double *y) {
    (void)context;
    *y = x * x + 1.0;
    return true;
}

static void expect_result(const char *name, numerical_result_t result,
                          double expected, double tolerance) {
    if (result.status != CALC_OK || fabs(result.value - expected) > tolerance) {
        printf("FAIL: %s status=%d x=%.12g value=%.12g expected=%.12g\n",
               name, result.status, result.x, result.value, expected);
        failures++;
    }
}

int main(void) {
    numerical_options_t options = numerical_default_options();

    numerical_result_t result = numerical_root_interval(
        quadratic, NULL, 0.0, 2.0, options);
    if (result.status != CALC_OK || fabs(result.x - sqrt(2.0)) > 1e-8) {
        printf("FAIL: interval root status=%d x=%.12g\n", result.status, result.x);
        failures++;
    }

    result = numerical_root_guess(cosine_fixed_point, NULL, 1.0, options);
    if (result.status != CALC_OK || fabs(result.x - 0.7390851332) > 1e-8) {
        printf("FAIL: guessed root status=%d x=%.12g\n", result.status, result.x);
        failures++;
    }

    result = numerical_intersection_interval(
        quadratic, NULL, line_function, NULL, 1.0, 2.0, options);
    double intersection = (-1.0 + sqrt(17.0)) * 0.5;
    if (result.status != CALC_OK || fabs(result.x - intersection) > 1e-8) {
        printf("FAIL: intersection status=%d x=%.12g\n", result.status, result.x);
        failures++;
    }

    result = numerical_derivative(sine_function, NULL, 0.0, options);
    expect_result("derivative", result, 1.0, 1e-7);

    result = numerical_integral(sine_function, NULL, 0.0, TEST_PI, options);
    expect_result("integral", result, 2.0, 1e-8);

    numerical_extremum_t extrema[8];
    calc_status_t status = CALC_OK;
    size_t count = numerical_find_extrema(
        sine_function, NULL, -2.0 * TEST_PI, 2.0 * TEST_PI,
        options, extrema, 8, &status);
    if (status != CALC_OK || count != 4) {
        printf("FAIL: extrema status=%d count=%u\n",
               status, (unsigned int)count);
        failures++;
    }
    for (size_t i = 0; i < count; ++i) {
        double expected = extrema[i].kind == NUMERICAL_MAXIMUM ? 1.0 : -1.0;
        if (fabs(extrema[i].value - expected) > 1e-7) {
            printf("FAIL: extremum %u x=%.12g value=%.12g\n",
                   (unsigned int)i, extrema[i].x, extrema[i].value);
            failures++;
        }
    }

    result = numerical_root_interval(
        positive_quadratic, NULL, -1.0, 1.0, options);
    if (result.status != CALC_CONVERGENCE_ERROR) {
        printf("FAIL: missing bracket status=%d\n", result.status);
        failures++;
    }

    options.max_iterations = 1;
    options.tolerance = 1e-15;
    result = numerical_integral(sine_function, NULL, 0.0, TEST_PI, options);
    if (result.status != CALC_CONVERGENCE_ERROR) {
        printf("FAIL: integration limit status=%d\n", result.status);
        failures++;
    }

    if (failures) {
        printf("%d numerical analysis test(s) failed\n", failures);
        return 1;
    }
    printf("numerical analysis tests passed\n");
    return 0;
}
