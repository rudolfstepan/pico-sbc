#ifndef STATISTICS_ENGINE_H
#define STATISTICS_ENGINE_H

#include <stdbool.h>
#include <stddef.h>

#define STATISTICS_CAPACITY 32u
#define STATISTICS_HISTOGRAM_BINS 8u

typedef enum {
    STATISTICS_STATUS_OK,
    STATISTICS_STATUS_EMPTY,
    STATISTICS_STATUS_FULL,
    STATISTICS_STATUS_INDEX,
    STATISTICS_STATUS_NONFINITE,
    STATISTICS_STATUS_NEED_TWO_VARIABLES,
    STATISTICS_STATUS_VARIANCE_ZERO
} statistics_status_t;

typedef struct {
    double x[STATISTICS_CAPACITY];
    double y[STATISTICS_CAPACITY];
    size_t count;
    bool two_variable;
} statistics_dataset_t;

typedef struct {
    size_t count;
    double mean;
    double median;
    double minimum;
    double maximum;
    double population_stddev;
    double sample_stddev;
} statistics_summary_t;

typedef struct {
    double slope;
    double intercept;
    double correlation;
} statistics_regression_t;

void statistics_engine_init(statistics_dataset_t *dataset);
void statistics_engine_clear(statistics_dataset_t *dataset);
void statistics_engine_set_two_variable(statistics_dataset_t *dataset,
                                        bool enabled);
statistics_status_t statistics_engine_add(statistics_dataset_t *dataset,
                                          double x, double y);
statistics_status_t statistics_engine_remove(statistics_dataset_t *dataset,
                                             size_t index);
statistics_status_t statistics_engine_summary(
    const statistics_dataset_t *dataset, bool use_y,
    statistics_summary_t *summary);
statistics_status_t statistics_engine_regression(
    const statistics_dataset_t *dataset,
    statistics_regression_t *regression);
statistics_status_t statistics_engine_histogram(
    const statistics_dataset_t *dataset,
    size_t counts[STATISTICS_HISTOGRAM_BINS],
    double *minimum, double *maximum);
const char *statistics_engine_status_text(statistics_status_t status);

#endif
