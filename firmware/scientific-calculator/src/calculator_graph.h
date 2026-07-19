#ifndef CALCULATOR_GRAPH_H
#define CALCULATOR_GRAPH_H

#include "calculator_widgets.h"

#include <stddef.h>

typedef struct {
    double x_center;
    double y_center;
    double x_span;
    double y_span;
} calculator_graph_t;

void calculator_graph_init(calculator_graph_t *graph);
void calculator_graph_pan(calculator_graph_t *graph,
                          double horizontal_fraction,
                          double vertical_fraction);
void calculator_graph_zoom(calculator_graph_t *graph, double factor);
void calculator_graph_render(const calculator_graph_t *graph,
                             const char *expression, double ans,
                             const calculator_widget_state_t *widget_state,
                             char *message, size_t message_size);

#endif
