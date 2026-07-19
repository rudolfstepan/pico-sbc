#ifndef CALCULATOR_GRAPH_H
#define CALCULATOR_GRAPH_H

#include "calculator_widgets.h"
#include "graph_model.h"

#include <stddef.h>

typedef graph_model_t calculator_graph_t;

typedef enum {
    CALCULATOR_GRAPH_ROOT,
    CALCULATOR_GRAPH_INTERSECTION,
    CALCULATOR_GRAPH_DERIVATIVE,
    CALCULATOR_GRAPH_INTEGRAL,
    CALCULATOR_GRAPH_EXTREMA
} calculator_graph_analysis_t;

void calculator_graph_init(calculator_graph_t *graph);
void calculator_graph_pan(calculator_graph_t *graph,
                          double horizontal_fraction,
                          double vertical_fraction);
void calculator_graph_zoom(calculator_graph_t *graph, double factor);
calc_status_t calculator_graph_auto_scale(calculator_graph_t *graph,
                                          double ans);
calc_status_t calculator_graph_analyze(calculator_graph_t *graph, double ans,
                                       calculator_graph_analysis_t analysis);
void calculator_graph_render(const calculator_graph_t *graph,
                             double ans,
                             const calculator_widget_state_t *widget_state,
                             char *message, size_t message_size);

#endif
