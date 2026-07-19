#ifndef GRAPH_MODEL_H
#define GRAPH_MODEL_H

#include "calculation_status.h"

#include <stdbool.h>
#include <stddef.h>

#define GRAPH_FUNCTION_COUNT 3
#define GRAPH_EXPRESSION_CAPACITY 96
#define GRAPH_ANALYSIS_TEXT_CAPACITY 80

typedef enum {
    GRAPH_VIEW_PLOT,
    GRAPH_VIEW_MENU,
    GRAPH_VIEW_ANALYSIS,
    GRAPH_VIEW_ANALYSIS_MORE,
    GRAPH_VIEW_TABLE,
    GRAPH_VIEW_RANGE
} graph_view_t;

typedef struct {
    char expression[GRAPH_EXPRESSION_CAPACITY];
    bool active;
} graph_function_t;

typedef struct {
    double x;
    double y;
    size_t first_function;
    size_t second_function;
} graph_marker_t;

typedef struct {
    graph_function_t functions[GRAPH_FUNCTION_COUNT];
    size_t selected_function;
    graph_view_t view;
    double x_center;
    double y_center;
    double x_span;
    double y_span;
    bool trace_enabled;
    double trace_x;
    double table_x;
    double table_step;
    char analysis_text[GRAPH_ANALYSIS_TEXT_CAPACITY];
    bool custom_analysis_interval;
    double analysis_left;
    double analysis_right;
    double analysis_tolerance;
    unsigned int analysis_max_iterations;
} graph_model_t;

typedef bool (*graph_evaluate_fn)(void *context, size_t function_index,
                                  double x, double *y);

void graph_model_init(graph_model_t *model);
bool graph_model_set_function(graph_model_t *model, size_t index,
                              const char *expression);
bool graph_model_select_function(graph_model_t *model, size_t index);
bool graph_model_toggle_function(graph_model_t *model, size_t index);
void graph_model_set_view(graph_model_t *model, graph_view_t view);
void graph_model_pan(graph_model_t *model, double horizontal_fraction,
                     double vertical_fraction);
void graph_model_zoom(graph_model_t *model, double factor);
bool graph_model_set_range(graph_model_t *model,
                           double x_min, double x_max,
                           double y_min, double y_max);
void graph_model_reset_range(graph_model_t *model);
void graph_model_move_trace(graph_model_t *model, double steps);
void graph_model_scroll_table(graph_model_t *model, double rows);
void graph_model_scale_table_step(graph_model_t *model, double factor);
void graph_model_clear_analysis(graph_model_t *model);
void graph_model_set_analysis_bound(graph_model_t *model, bool left,
                                    double value);
void graph_model_use_view_interval(graph_model_t *model);
void graph_model_analysis_interval(const graph_model_t *model,
                                   double *left, double *right);
void graph_model_cycle_analysis_tolerance(graph_model_t *model);
calc_status_t graph_model_auto_scale(graph_model_t *model,
                                     graph_evaluate_fn evaluate,
                                     void *context);
size_t graph_model_find_roots(const graph_model_t *model,
                              graph_evaluate_fn evaluate, void *context,
                              graph_marker_t *markers, size_t capacity);
size_t graph_model_find_intersections(const graph_model_t *model,
                                      graph_evaluate_fn evaluate,
                                      void *context,
                                      graph_marker_t *markers,
                                      size_t capacity);

#endif
