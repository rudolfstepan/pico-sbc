#include "statistics_engine.h"

#include <float.h>
#include <math.h>
#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int close_enough(double actual, double expected) {
    return fabs(actual - expected) < 1e-10;
}

int main(void) {
    statistics_dataset_t data;
    statistics_engine_init(&data);
    CHECK(statistics_engine_add(&data, 1.0, 0.0) == STATISTICS_STATUS_OK);
    CHECK(statistics_engine_add(&data, 2.0, 0.0) == STATISTICS_STATUS_OK);
    CHECK(statistics_engine_add(&data, 3.0, 0.0) == STATISTICS_STATUS_OK);
    CHECK(statistics_engine_add(&data, 4.0, 0.0) == STATISTICS_STATUS_OK);

    statistics_summary_t summary;
    CHECK(statistics_engine_summary(&data, false, &summary) ==
          STATISTICS_STATUS_OK);
    CHECK(summary.count == 4);
    CHECK(close_enough(summary.mean, 2.5));
    CHECK(close_enough(summary.median, 2.5));
    CHECK(summary.minimum == 1.0 && summary.maximum == 4.0);
    CHECK(close_enough(summary.population_stddev, sqrt(1.25)));
    CHECK(close_enough(summary.sample_stddev, sqrt(5.0 / 3.0)));

    size_t bins[STATISTICS_HISTOGRAM_BINS];
    double minimum = 0.0;
    double maximum = 0.0;
    CHECK(statistics_engine_histogram(&data, bins, &minimum, &maximum) ==
          STATISTICS_STATUS_OK);
    size_t total = 0;
    for (size_t i = 0; i < STATISTICS_HISTOGRAM_BINS; ++i) total += bins[i];
    CHECK(total == 4 && minimum == 1.0 && maximum == 4.0);

    statistics_engine_set_two_variable(&data, true);
    CHECK(data.count == 0 && data.two_variable);
    for (int x = 0; x < 5; ++x) {
        CHECK(statistics_engine_add(&data, x, 2.0 * x + 1.0) ==
              STATISTICS_STATUS_OK);
    }
    statistics_regression_t regression;
    CHECK(statistics_engine_regression(&data, &regression) ==
          STATISTICS_STATUS_OK);
    CHECK(close_enough(regression.slope, 2.0));
    CHECK(close_enough(regression.intercept, 1.0));
    CHECK(close_enough(regression.correlation, 1.0));
    CHECK(statistics_engine_summary(&data, true, &summary) ==
          STATISTICS_STATUS_OK);
    CHECK(summary.mean == 5.0);

    CHECK(statistics_engine_remove(&data, 2) == STATISTICS_STATUS_OK);
    CHECK(data.count == 4 && data.x[2] == 3.0);

    statistics_engine_set_two_variable(&data, false);
    CHECK(statistics_engine_add(&data, DBL_MAX * 0.5, 0.0) ==
          STATISTICS_STATUS_OK);
    CHECK(statistics_engine_add(&data, DBL_MAX * 0.5, 0.0) ==
          STATISTICS_STATUS_OK);
    CHECK(statistics_engine_summary(&data, false, &summary) ==
          STATISTICS_STATUS_OK);
    CHECK(summary.mean == DBL_MAX * 0.5 && summary.population_stddev == 0.0);

    statistics_engine_clear(&data);
    CHECK(data.count == 0 && !data.two_variable);
    CHECK(statistics_engine_summary(&data, false, &summary) ==
          STATISTICS_STATUS_EMPTY);
    CHECK(statistics_engine_add(&data, INFINITY, 0.0) ==
          STATISTICS_STATUS_NONFINITE);

    puts("statistics engine tests passed");
    return 0;
}
