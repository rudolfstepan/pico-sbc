#include "numerical_analysis.h"

#include <float.h>
#include <math.h>
#include <string.h>

#define NUMERICAL_MAX_SAMPLES 240u
#define GOLDEN_RATIO_PART 0.3819660112501051

static numerical_result_t result_with_status(calc_status_t status) {
    numerical_result_t result;
    memset(&result, 0, sizeof result);
    result.status = status;
    return result;
}

static bool valid_options(numerical_options_t options) {
    return options.tolerance > 0.0 && isfinite(options.tolerance) &&
           options.max_iterations > 0 && options.sample_count >= 4;
}

numerical_options_t numerical_default_options(void) {
    numerical_options_t options = {
        .tolerance = 1e-9,
        .max_iterations = 64,
        .sample_count = 160,
    };
    return options;
}

numerical_result_t numerical_root_interval(numerical_function_t function,
                                          void *context,
                                          double left, double right,
                                          numerical_options_t options) {
    numerical_result_t result = result_with_status(CALC_CONVERGENCE_ERROR);
    if (!function || !valid_options(options) || !isfinite(left) ||
        !isfinite(right) || !(right > left)) {
        result.status = CALC_RANGE_ERROR;
        return result;
    }

    double left_value = 0.0;
    double right_value = 0.0;
    if (!function(context, left, &left_value) ||
        !function(context, right, &right_value) ||
        !isfinite(left_value) || !isfinite(right_value)) {
        result.status = CALC_DOMAIN_ERROR;
        return result;
    }
    if (fabs(left_value) <= options.tolerance) {
        result.status = CALC_OK;
        result.x = left;
        result.value = left_value;
        return result;
    }
    if (fabs(right_value) <= options.tolerance) {
        result.status = CALC_OK;
        result.x = right;
        result.value = right_value;
        return result;
    }
    if ((left_value < 0.0) == (right_value < 0.0)) return result;

    for (unsigned int iteration = 1;
         iteration <= options.max_iterations; ++iteration) {
        double middle = left + (right - left) * 0.5;
        double middle_value = 0.0;
        if (!function(context, middle, &middle_value) ||
            !isfinite(middle_value)) {
            result.status = CALC_DOMAIN_ERROR;
            result.iterations = iteration;
            return result;
        }
        result.x = middle;
        result.value = middle_value;
        result.iterations = iteration;
        if (fabs(middle_value) <= options.tolerance ||
            (right - left) * 0.5 <=
                options.tolerance * fmax(1.0, fabs(middle))) {
            result.status = CALC_OK;
            return result;
        }
        if ((left_value < 0.0) != (middle_value < 0.0)) {
            right = middle;
        } else {
            left = middle;
            left_value = middle_value;
        }
    }
    return result;
}

static bool derivative_estimate(numerical_function_t function, void *context,
                                double x, double step, double *value) {
    double left = 0.0;
    double right = 0.0;
    if (!function(context, x - step, &left) ||
        !function(context, x + step, &right) ||
        !isfinite(left) || !isfinite(right)) {
        return false;
    }
    *value = (right - left) / (2.0 * step);
    return isfinite(*value);
}

numerical_result_t numerical_root_guess(numerical_function_t function,
                                       void *context, double guess,
                                       numerical_options_t options) {
    numerical_result_t result = result_with_status(CALC_CONVERGENCE_ERROR);
    if (!function || !valid_options(options) || !isfinite(guess)) {
        result.status = CALC_RANGE_ERROR;
        return result;
    }

    double x = guess;
    for (unsigned int iteration = 1;
         iteration <= options.max_iterations; ++iteration) {
        double value = 0.0;
        if (!function(context, x, &value) || !isfinite(value)) {
            result.status = CALC_DOMAIN_ERROR;
            result.iterations = iteration;
            return result;
        }
        result.x = x;
        result.value = value;
        result.iterations = iteration;
        if (fabs(value) <= options.tolerance) {
            result.status = CALC_OK;
            return result;
        }

        double step = cbrt(DBL_EPSILON) * fmax(1.0, fabs(x));
        double derivative = 0.0;
        if (!derivative_estimate(function, context, x, step, &derivative) ||
            fabs(derivative) < DBL_EPSILON) {
            return result;
        }
        double next = x - value / derivative;
        if (!isfinite(next)) return result;
        if (fabs(next - x) <= options.tolerance * fmax(1.0, fabs(next))) {
            x = next;
            if (!function(context, x, &result.value) ||
                !isfinite(result.value)) {
                result.status = CALC_DOMAIN_ERROR;
                return result;
            }
            result.x = x;
            result.status = fabs(result.value) <= sqrt(options.tolerance)
                ? CALC_OK : CALC_CONVERGENCE_ERROR;
            return result;
        }
        x = next;
    }
    return result;
}

typedef struct {
    numerical_function_t first;
    void *first_context;
    numerical_function_t second;
    void *second_context;
} intersection_context_t;

static bool evaluate_difference(void *context, double x, double *value) {
    intersection_context_t *intersection = context;
    double first = 0.0;
    double second = 0.0;
    if (!intersection->first(intersection->first_context, x, &first) ||
        !intersection->second(intersection->second_context, x, &second)) {
        return false;
    }
    *value = first - second;
    return isfinite(*value);
}

numerical_result_t numerical_intersection_interval(
    numerical_function_t first, void *first_context,
    numerical_function_t second, void *second_context,
    double left, double right, numerical_options_t options) {
    if (!first || !second) return result_with_status(CALC_DOMAIN_ERROR);
    intersection_context_t context = {
        .first = first,
        .first_context = first_context,
        .second = second,
        .second_context = second_context,
    };
    numerical_result_t result = numerical_root_interval(
        evaluate_difference, &context, left, right, options);
    if (result.status == CALC_OK) {
        if (!first(first_context, result.x, &result.value) ||
            !isfinite(result.value)) {
            result.status = CALC_DOMAIN_ERROR;
        }
    }
    return result;
}

numerical_result_t numerical_derivative(numerical_function_t function,
                                        void *context, double x,
                                        numerical_options_t options) {
    numerical_result_t result = result_with_status(CALC_CONVERGENCE_ERROR);
    result.x = x;
    if (!function || !valid_options(options) || !isfinite(x)) {
        result.status = CALC_RANGE_ERROR;
        return result;
    }

    double step = cbrt(DBL_EPSILON) * fmax(1.0, fabs(x));
    double previous = 0.0;
    if (!derivative_estimate(function, context, x, step, &previous)) {
        result.status = CALC_DOMAIN_ERROR;
        return result;
    }
    for (unsigned int iteration = 1;
         iteration <= options.max_iterations; ++iteration) {
        step *= 0.5;
        double current = 0.0;
        if (!derivative_estimate(function, context, x, step, &current)) {
            result.status = CALC_DOMAIN_ERROR;
            result.iterations = iteration;
            return result;
        }
        double refined = current + (current - previous) / 3.0;
        result.value = refined;
        result.iterations = iteration;
        if (fabs(refined - previous) <=
            options.tolerance * fmax(1.0, fabs(refined))) {
            result.status = CALC_OK;
            return result;
        }
        previous = current;
        if (step <= DBL_EPSILON * fmax(1.0, fabs(x))) break;
    }
    return result;
}

static bool simpson_estimate(numerical_function_t function, void *context,
                             double left, double right, unsigned int segments,
                             double *estimate) {
    double step = (right - left) / segments;
    double left_value = 0.0;
    double right_value = 0.0;
    if (!function(context, left, &left_value) ||
        !function(context, right, &right_value) ||
        !isfinite(left_value) || !isfinite(right_value)) {
        return false;
    }
    double sum = left_value + right_value;
    for (unsigned int i = 1; i < segments; ++i) {
        double value = 0.0;
        if (!function(context, left + i * step, &value) || !isfinite(value)) {
            return false;
        }
        sum += (i & 1u ? 4.0 : 2.0) * value;
    }
    *estimate = sum * step / 3.0;
    return isfinite(*estimate);
}

numerical_result_t numerical_integral(numerical_function_t function,
                                      void *context,
                                      double left, double right,
                                      numerical_options_t options) {
    numerical_result_t result = result_with_status(CALC_CONVERGENCE_ERROR);
    if (!function || !valid_options(options) || !isfinite(left) ||
        !isfinite(right) || left == right) {
        result.status = CALC_RANGE_ERROR;
        return result;
    }
    double sign = 1.0;
    if (right < left) {
        double swap = left;
        left = right;
        right = swap;
        sign = -1.0;
    }

    unsigned int segments = 4;
    double previous = 0.0;
    if (!simpson_estimate(function, context, left, right,
                          segments, &previous)) {
        result.status = CALC_DOMAIN_ERROR;
        return result;
    }
    for (unsigned int iteration = 1;
         iteration <= options.max_iterations && segments <= 32768u;
         ++iteration) {
        segments *= 2u;
        double current = 0.0;
        if (!simpson_estimate(function, context, left, right,
                              segments, &current)) {
            result.status = CALC_DOMAIN_ERROR;
            result.iterations = iteration;
            return result;
        }
        result.x = right;
        result.value = sign * current;
        result.iterations = iteration;
        if (fabs(current - previous) <=
            15.0 * options.tolerance * fmax(1.0, fabs(current))) {
            result.status = CALC_OK;
            return result;
        }
        previous = current;
    }
    return result;
}

static bool refine_extremum(numerical_function_t function, void *context,
                            double left, double right,
                            numerical_extremum_kind_t kind,
                            numerical_options_t options,
                            numerical_extremum_t *extremum) {
    double sign = kind == NUMERICAL_MINIMUM ? 1.0 : -1.0;
    double first_x = left + GOLDEN_RATIO_PART * (right - left);
    double second_x = right - GOLDEN_RATIO_PART * (right - left);
    double first_value = 0.0;
    double second_value = 0.0;
    if (!function(context, first_x, &first_value) ||
        !function(context, second_x, &second_value)) {
        return false;
    }
    for (unsigned int iteration = 0; iteration < options.max_iterations;
         ++iteration) {
        if (right - left <= options.tolerance *
            fmax(1.0, fabs((left + right) * 0.5))) {
            break;
        }
        if (sign * first_value < sign * second_value) {
            right = second_x;
            second_x = first_x;
            second_value = first_value;
            first_x = left + GOLDEN_RATIO_PART * (right - left);
            if (!function(context, first_x, &first_value)) return false;
        } else {
            left = first_x;
            first_x = second_x;
            first_value = second_value;
            second_x = right - GOLDEN_RATIO_PART * (right - left);
            if (!function(context, second_x, &second_value)) return false;
        }
    }
    extremum->kind = kind;
    extremum->x = (left + right) * 0.5;
    return function(context, extremum->x, &extremum->value) &&
           isfinite(extremum->value);
}

size_t numerical_find_extrema(numerical_function_t function, void *context,
                              double left, double right,
                              numerical_options_t options,
                              numerical_extremum_t *extrema,
                              size_t capacity, calc_status_t *status) {
    if (status) *status = CALC_RANGE_ERROR;
    if (!function || !extrema || !capacity || !valid_options(options) ||
        !isfinite(left) || !isfinite(right) || !(right > left)) {
        return 0;
    }
    unsigned int samples = options.sample_count;
    if (samples > NUMERICAL_MAX_SAMPLES) samples = NUMERICAL_MAX_SAMPLES;
    double values[NUMERICAL_MAX_SAMPLES + 1];
    double step = (right - left) / samples;
    for (unsigned int i = 0; i <= samples; ++i) {
        if (!function(context, left + i * step, &values[i]) ||
            !isfinite(values[i])) {
            if (status) *status = CALC_DOMAIN_ERROR;
            return 0;
        }
    }

    size_t count = 0;
    for (unsigned int i = 1; i < samples && count < capacity; ++i) {
        numerical_extremum_kind_t kind;
        if (values[i] < values[i - 1] && values[i] <= values[i + 1]) {
            kind = NUMERICAL_MINIMUM;
        } else if (values[i] > values[i - 1] &&
                   values[i] >= values[i + 1]) {
            kind = NUMERICAL_MAXIMUM;
        } else {
            continue;
        }
        if (refine_extremum(function, context,
                            left + (i - 1u) * step,
                            left + (i + 1u) * step,
                            kind, options, &extrema[count])) {
            count++;
        }
    }
    if (status) *status = count ? CALC_OK : CALC_EMPTY;
    return count;
}
