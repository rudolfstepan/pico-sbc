#ifndef NUMERICAL_ANALYSIS_H
#define NUMERICAL_ANALYSIS_H

#include "calculation_status.h"

#include <stdbool.h>
#include <stddef.h>

typedef bool (*numerical_function_t)(void *context, double x, double *y);

typedef struct {
    double tolerance;
    unsigned int max_iterations;
    unsigned int sample_count;
} numerical_options_t;

typedef struct {
    calc_status_t status;
    double x;
    double value;
    unsigned int iterations;
} numerical_result_t;

typedef enum {
    NUMERICAL_MINIMUM,
    NUMERICAL_MAXIMUM
} numerical_extremum_kind_t;

typedef struct {
    numerical_extremum_kind_t kind;
    double x;
    double value;
} numerical_extremum_t;

numerical_options_t numerical_default_options(void);
numerical_result_t numerical_root_interval(numerical_function_t function,
                                          void *context,
                                          double left, double right,
                                          numerical_options_t options);
numerical_result_t numerical_root_guess(numerical_function_t function,
                                       void *context, double guess,
                                       numerical_options_t options);
numerical_result_t numerical_intersection_interval(
    numerical_function_t first, void *first_context,
    numerical_function_t second, void *second_context,
    double left, double right, numerical_options_t options);
numerical_result_t numerical_derivative(numerical_function_t function,
                                        void *context, double x,
                                        numerical_options_t options);
numerical_result_t numerical_integral(numerical_function_t function,
                                      void *context,
                                      double left, double right,
                                      numerical_options_t options);
size_t numerical_find_extrema(numerical_function_t function, void *context,
                              double left, double right,
                              numerical_options_t options,
                              numerical_extremum_t *extrema,
                              size_t capacity, calc_status_t *status);

#endif
