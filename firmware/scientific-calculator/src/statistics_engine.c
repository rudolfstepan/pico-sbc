#include "statistics_engine.h"

#include <math.h>
#include <string.h>

void statistics_engine_init(statistics_dataset_t *dataset) {
    if (!dataset) return;
    memset(dataset, 0, sizeof *dataset);
}

void statistics_engine_clear(statistics_dataset_t *dataset) {
    if (!dataset) return;
    bool two_variable = dataset->two_variable;
    memset(dataset, 0, sizeof *dataset);
    dataset->two_variable = two_variable;
}

void statistics_engine_set_two_variable(statistics_dataset_t *dataset,
                                        bool enabled) {
    if (!dataset || dataset->two_variable == enabled) return;
    statistics_engine_clear(dataset);
    dataset->two_variable = enabled;
}

statistics_status_t statistics_engine_add(statistics_dataset_t *dataset,
                                          double x, double y) {
    if (!dataset || !isfinite(x) || !isfinite(y)) {
        return STATISTICS_STATUS_NONFINITE;
    }
    if (dataset->count >= STATISTICS_CAPACITY) return STATISTICS_STATUS_FULL;
    dataset->x[dataset->count] = x;
    dataset->y[dataset->count] = dataset->two_variable ? y : 0.0;
    dataset->count++;
    return STATISTICS_STATUS_OK;
}

statistics_status_t statistics_engine_remove(statistics_dataset_t *dataset,
                                             size_t index) {
    if (!dataset || index >= dataset->count) return STATISTICS_STATUS_INDEX;
    size_t remaining = dataset->count - index - 1;
    if (remaining) {
        memmove(&dataset->x[index], &dataset->x[index + 1],
                remaining * sizeof dataset->x[0]);
        memmove(&dataset->y[index], &dataset->y[index + 1],
                remaining * sizeof dataset->y[0]);
    }
    dataset->count--;
    dataset->x[dataset->count] = 0.0;
    dataset->y[dataset->count] = 0.0;
    return STATISTICS_STATUS_OK;
}

static void sort_values(double *values, size_t count) {
    for (size_t i = 1; i < count; ++i) {
        double current = values[i];
        size_t position = i;
        while (position && values[position - 1] > current) {
            values[position] = values[position - 1];
            position--;
        }
        values[position] = current;
    }
}

statistics_status_t statistics_engine_summary(
    const statistics_dataset_t *dataset, bool use_y,
    statistics_summary_t *summary) {
    if (!dataset || !summary || !dataset->count) {
        return STATISTICS_STATUS_EMPTY;
    }
    if (use_y && !dataset->two_variable) {
        return STATISTICS_STATUS_NEED_TWO_VARIABLES;
    }
    const double *values = use_y ? dataset->y : dataset->x;
    double sorted[STATISTICS_CAPACITY];
    double mean = 0.0;
    double squared = 0.0;
    double minimum = values[0];
    double maximum = values[0];
    for (size_t i = 0; i < dataset->count; ++i) {
        sorted[i] = values[i];
        double delta = values[i] - mean;
        mean += delta / (double)(i + 1);
        squared += delta * (values[i] - mean);
        if (values[i] < minimum) minimum = values[i];
        if (values[i] > maximum) maximum = values[i];
    }
    sort_values(sorted, dataset->count);
    size_t middle = dataset->count / 2;
    double median = dataset->count & 1u
        ? sorted[middle]
        : sorted[middle - 1] * 0.5 + sorted[middle] * 0.5;
    *summary = (statistics_summary_t){
        .count = dataset->count,
        .mean = mean,
        .median = median,
        .minimum = minimum,
        .maximum = maximum,
        .population_stddev = sqrt(squared / (double)dataset->count),
        .sample_stddev = dataset->count > 1
            ? sqrt(squared / (double)(dataset->count - 1)) : 0.0,
    };
    return isfinite(mean) && isfinite(squared)
        ? STATISTICS_STATUS_OK : STATISTICS_STATUS_NONFINITE;
}

statistics_status_t statistics_engine_regression(
    const statistics_dataset_t *dataset,
    statistics_regression_t *regression) {
    if (!dataset || !regression || !dataset->two_variable) {
        return STATISTICS_STATUS_NEED_TWO_VARIABLES;
    }
    if (dataset->count < 2) return STATISTICS_STATUS_EMPTY;
    double mean_x = 0.0;
    double mean_y = 0.0;
    double xx = 0.0;
    double yy = 0.0;
    double xy = 0.0;
    for (size_t i = 0; i < dataset->count; ++i) {
        double dx = dataset->x[i] - mean_x;
        double dy = dataset->y[i] - mean_y;
        mean_x += dx / (double)(i + 1);
        mean_y += dy / (double)(i + 1);
        xx += dx * (dataset->x[i] - mean_x);
        yy += dy * (dataset->y[i] - mean_y);
        xy += dx * (dataset->y[i] - mean_y);
    }
    if (!isfinite(xx) || !isfinite(yy) || !isfinite(xy)) {
        return STATISTICS_STATUS_NONFINITE;
    }
    if (xx <= 0.0 || yy <= 0.0) return STATISTICS_STATUS_VARIANCE_ZERO;
    double slope = xy / xx;
    *regression = (statistics_regression_t){
        .slope = slope,
        .intercept = mean_y - slope * mean_x,
        .correlation = xy / (sqrt(xx) * sqrt(yy)),
    };
    return isfinite(regression->slope) && isfinite(regression->intercept) &&
           isfinite(regression->correlation)
        ? STATISTICS_STATUS_OK : STATISTICS_STATUS_NONFINITE;
}

statistics_status_t statistics_engine_histogram(
    const statistics_dataset_t *dataset,
    size_t counts[STATISTICS_HISTOGRAM_BINS],
    double *minimum, double *maximum) {
    statistics_summary_t summary;
    statistics_status_t status = statistics_engine_summary(dataset, false,
                                                            &summary);
    if (status != STATISTICS_STATUS_OK || !counts || !minimum || !maximum) {
        return status;
    }
    memset(counts, 0, STATISTICS_HISTOGRAM_BINS * sizeof counts[0]);
    *minimum = summary.minimum;
    *maximum = summary.maximum;
    if (summary.minimum == summary.maximum) {
        counts[STATISTICS_HISTOGRAM_BINS / 2] = dataset->count;
        return STATISTICS_STATUS_OK;
    }
    double width = (summary.maximum - summary.minimum) /
                   (double)STATISTICS_HISTOGRAM_BINS;
    for (size_t i = 0; i < dataset->count; ++i) {
        size_t bin = (size_t)((dataset->x[i] - summary.minimum) / width);
        if (bin >= STATISTICS_HISTOGRAM_BINS) {
            bin = STATISTICS_HISTOGRAM_BINS - 1;
        }
        counts[bin]++;
    }
    return STATISTICS_STATUS_OK;
}

const char *statistics_engine_status_text(statistics_status_t status) {
    switch (status) {
        case STATISTICS_STATUS_OK: return "OK";
        case STATISTICS_STATUS_EMPTY: return "NO DATA";
        case STATISTICS_STATUS_FULL: return "DATA FULL";
        case STATISTICS_STATUS_INDEX: return "INVALID ROW";
        case STATISTICS_STATUS_NONFINITE: return "NUMBER RANGE";
        case STATISTICS_STATUS_NEED_TWO_VARIABLES: return "2VAR REQUIRED";
        case STATISTICS_STATUS_VARIANCE_ZERO: return "ZERO VARIANCE";
        default: return "STAT ERROR";
    }
}
