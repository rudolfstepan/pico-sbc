#include "graph_model.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

static bool evaluate_polynomial(void *context, size_t function,
                                double x, double *y) {
    (void)context;
    if (function == 0) *y = x * x - 4.0;
    else if (function == 1) *y = x;
    else if (function == 2) *y = -x;
    else return false;
    return true;
}

static void expect_close(double actual, double expected, double tolerance,
                         const char *label) {
    if (fabs(actual - expected) > tolerance) {
        printf("FAIL: %s %.12g expected %.12g\n", label, actual, expected);
        failures++;
    }
}

int main(void) {
    graph_model_t model;
    graph_model_init(&model);
    if (!model.functions[0].active || model.functions[1].active ||
        model.selected_function != 0 || model.view != GRAPH_VIEW_PLOT) {
        printf("FAIL: initial state\n");
        failures++;
    }

    if (!graph_model_set_function(&model, 0, "x^2-4") ||
        !graph_model_set_function(&model, 1, "x") ||
        !graph_model_set_function(&model, 2, "-x")) {
        printf("FAIL: function storage\n");
        failures++;
    }
    if (!graph_model_select_function(&model, 2) ||
        strcmp(model.functions[2].expression, "-x") != 0) {
        printf("FAIL: function selection\n");
        failures++;
    }
    if (!graph_model_toggle_function(&model, 2) || model.functions[2].active) {
        printf("FAIL: function toggle\n");
        failures++;
    }
    graph_model_toggle_function(&model, 2);

    if (!graph_model_set_range(&model, -5.0, 5.0, -8.0, 8.0)) failures++;
    graph_model_move_trace(&model, 4.0);
    expect_close(model.trace_x, 0.5, 1e-12, "trace");
    graph_model_scroll_table(&model, 3.0);
    expect_close(model.table_x, -2.0, 1e-12, "table scroll");
    graph_model_scale_table_step(&model, 0.5);
    expect_close(model.table_step, 0.5, 1e-12, "table step");

    double interval_left = 0.0;
    double interval_right = 0.0;
    graph_model_analysis_interval(&model, &interval_left, &interval_right);
    expect_close(interval_left, -5.0, 1e-12, "view interval left");
    expect_close(interval_right, 5.0, 1e-12, "view interval right");
    graph_model_set_analysis_bound(&model, true, 3.0);
    graph_model_set_analysis_bound(&model, false, -2.0);
    graph_model_analysis_interval(&model, &interval_left, &interval_right);
    expect_close(interval_left, -2.0, 1e-12, "custom interval left");
    expect_close(interval_right, 3.0, 1e-12, "custom interval right");
    graph_model_cycle_analysis_tolerance(&model);
    expect_close(model.analysis_tolerance, 1e-12, 1e-18,
                 "analysis tolerance");
    graph_model_use_view_interval(&model);
    if (model.custom_analysis_interval) failures++;

    if (graph_model_auto_scale(&model, evaluate_polynomial, NULL) != CALC_OK) {
        printf("FAIL: auto scale status\n");
        failures++;
    }
    if (!(model.y_span > 20.0 && fabs(model.y_center - 8.0) < 1e-12)) {
        printf("FAIL: auto scale range %.12g %.12g\n",
               model.y_center, model.y_span);
        failures++;
    }

    graph_marker_t markers[12];
    size_t roots = graph_model_find_roots(&model, evaluate_polynomial, NULL,
                                          markers, 12);
    bool negative_root = false;
    bool positive_root = false;
    for (size_t i = 0; i < roots; ++i) {
        if (markers[i].first_function == 0 && fabs(markers[i].x + 2.0) < 1e-6) {
            negative_root = true;
        }
        if (markers[i].first_function == 0 && fabs(markers[i].x - 2.0) < 1e-6) {
            positive_root = true;
        }
    }
    if (!negative_root || !positive_root) {
        printf("FAIL: roots count=%u\n", (unsigned int)roots);
        failures++;
    }
    if (roots != 4) {
        printf("FAIL: expected four unique roots, got %u\n",
               (unsigned int)roots);
        failures++;
    }

    size_t intersections = graph_model_find_intersections(
        &model, evaluate_polynomial, NULL, markers, 12);
    if (intersections != 5) {
        printf("FAIL: intersections count=%u\n",
               (unsigned int)intersections);
        failures++;
    }

    if (graph_model_set_range(&model, 1.0, 1.0, -1.0, 1.0)) {
        printf("FAIL: invalid range accepted\n");
        failures++;
    }

    if (failures) {
        printf("%d graph model test(s) failed\n", failures);
        return 1;
    }
    printf("graph model tests passed\n");
    return 0;
}
