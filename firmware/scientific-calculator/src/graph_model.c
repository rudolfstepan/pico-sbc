#include "graph_model.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define GRAPH_DEFAULT_X_SPAN 20.0
#define GRAPH_DEFAULT_Y_SPAN 10.0
#define GRAPH_SCAN_STEPS 240
#define GRAPH_REFINE_STEPS 32

void graph_model_init(graph_model_t *model) {
    memset(model, 0, sizeof *model);
    model->functions[0].active = true;
    model->view = GRAPH_VIEW_PLOT;
    model->x_span = GRAPH_DEFAULT_X_SPAN;
    model->y_span = GRAPH_DEFAULT_Y_SPAN;
    model->table_step = 1.0;
    model->analysis_tolerance = 1e-9;
    model->analysis_max_iterations = 64;
}

bool graph_model_set_function(graph_model_t *model, size_t index,
                              const char *expression) {
    if (index >= GRAPH_FUNCTION_COUNT || !expression ||
        strlen(expression) >= GRAPH_EXPRESSION_CAPACITY) {
        return false;
    }
    snprintf(model->functions[index].expression,
             sizeof model->functions[index].expression, "%s", expression);
    model->functions[index].active = expression[0] != '\0';
    graph_model_clear_analysis(model);
    return true;
}

bool graph_model_select_function(graph_model_t *model, size_t index) {
    if (index >= GRAPH_FUNCTION_COUNT) return false;
    model->selected_function = index;
    graph_model_clear_analysis(model);
    return true;
}

bool graph_model_toggle_function(graph_model_t *model, size_t index) {
    if (index >= GRAPH_FUNCTION_COUNT ||
        !model->functions[index].expression[0]) {
        return false;
    }
    model->functions[index].active = !model->functions[index].active;
    graph_model_clear_analysis(model);
    return true;
}

void graph_model_set_view(graph_model_t *model, graph_view_t view) {
    model->view = view;
}

void graph_model_pan(graph_model_t *model, double horizontal_fraction,
                     double vertical_fraction) {
    model->x_center += model->x_span * horizontal_fraction;
    model->y_center += model->y_span * vertical_fraction;
    model->trace_x += model->x_span * horizontal_fraction;
    model->table_x += model->x_span * horizontal_fraction;
    graph_model_clear_analysis(model);
}

void graph_model_zoom(graph_model_t *model, double factor) {
    if (!(factor > 0.0) || !isfinite(factor)) return;
    double x_span = model->x_span * factor;
    double y_span = model->y_span * factor;
    if (!isfinite(x_span) || !isfinite(y_span) ||
        x_span < 1e-12 || y_span < 1e-12) {
        return;
    }
    model->x_span = x_span;
    model->y_span = y_span;
    graph_model_clear_analysis(model);
}

bool graph_model_set_range(graph_model_t *model,
                           double x_min, double x_max,
                           double y_min, double y_max) {
    if (!isfinite(x_min) || !isfinite(x_max) ||
        !isfinite(y_min) || !isfinite(y_max) ||
        !(x_max > x_min) || !(y_max > y_min)) {
        return false;
    }
    model->x_center = (x_min + x_max) * 0.5;
    model->y_center = (y_min + y_max) * 0.5;
    model->x_span = x_max - x_min;
    model->y_span = y_max - y_min;
    model->trace_x = model->x_center;
    model->table_x = x_min;
    graph_model_clear_analysis(model);
    return true;
}

void graph_model_reset_range(graph_model_t *model) {
    model->x_center = 0.0;
    model->y_center = 0.0;
    model->x_span = GRAPH_DEFAULT_X_SPAN;
    model->y_span = GRAPH_DEFAULT_Y_SPAN;
    model->trace_x = 0.0;
    model->table_x = -10.0;
    model->table_step = 1.0;
    model->custom_analysis_interval = false;
    graph_model_clear_analysis(model);
}

void graph_model_move_trace(graph_model_t *model, double steps) {
    double step = model->x_span / 80.0;
    model->trace_x += step * steps;
    double x_min = model->x_center - model->x_span * 0.5;
    double x_max = model->x_center + model->x_span * 0.5;
    if (model->trace_x < x_min) model->trace_x = x_min;
    if (model->trace_x > x_max) model->trace_x = x_max;
    graph_model_clear_analysis(model);
}

void graph_model_scroll_table(graph_model_t *model, double rows) {
    model->table_x += model->table_step * rows;
}

void graph_model_scale_table_step(graph_model_t *model, double factor) {
    if (!(factor > 0.0) || !isfinite(factor)) return;
    double step = model->table_step * factor;
    if (isfinite(step) && step >= 1e-9 && step <= 1e9) {
        model->table_step = step;
    }
}

void graph_model_clear_analysis(graph_model_t *model) {
    model->analysis_text[0] = '\0';
}

void graph_model_set_analysis_bound(graph_model_t *model, bool left,
                                    double value) {
    if (!isfinite(value)) return;
    if (!model->custom_analysis_interval) {
        model->analysis_left = model->x_center - model->x_span * 0.5;
        model->analysis_right = model->x_center + model->x_span * 0.5;
        model->custom_analysis_interval = true;
    }
    if (left) {
        model->analysis_left = value;
    } else {
        model->analysis_right = value;
    }
    graph_model_clear_analysis(model);
}

void graph_model_use_view_interval(graph_model_t *model) {
    model->custom_analysis_interval = false;
    graph_model_clear_analysis(model);
}

void graph_model_analysis_interval(const graph_model_t *model,
                                   double *left, double *right) {
    double first = model->custom_analysis_interval
        ? model->analysis_left : model->x_center - model->x_span * 0.5;
    double second = model->custom_analysis_interval
        ? model->analysis_right : model->x_center + model->x_span * 0.5;
    if (first <= second) {
        *left = first;
        *right = second;
    } else {
        *left = second;
        *right = first;
    }
}

void graph_model_cycle_analysis_tolerance(graph_model_t *model) {
    if (model->analysis_tolerance > 5e-8) {
        model->analysis_tolerance = 1e-9;
    } else if (model->analysis_tolerance > 5e-11) {
        model->analysis_tolerance = 1e-12;
    } else {
        model->analysis_tolerance = 1e-6;
    }
    graph_model_clear_analysis(model);
}

calc_status_t graph_model_auto_scale(graph_model_t *model,
                                     graph_evaluate_fn evaluate,
                                     void *context) {
    if (!evaluate) return CALC_DOMAIN_ERROR;
    double x_min = model->x_center - model->x_span * 0.5;
    double y_min = 0.0;
    double y_max = 0.0;
    bool found = false;

    for (size_t function = 0; function < GRAPH_FUNCTION_COUNT; ++function) {
        if (!model->functions[function].active) continue;
        for (int sample = 0; sample <= GRAPH_SCAN_STEPS; ++sample) {
            double x = x_min + model->x_span * sample / GRAPH_SCAN_STEPS;
            double y = 0.0;
            if (!evaluate(context, function, x, &y) || !isfinite(y)) continue;
            if (!found) {
                y_min = y;
                y_max = y;
                found = true;
            } else {
                if (y < y_min) y_min = y;
                if (y > y_max) y_max = y;
            }
        }
    }

    if (!found) return CALC_DOMAIN_ERROR;
    double span = y_max - y_min;
    if (span < 1e-9) {
        span = fmax(2.0, fabs(y_min) * 0.2);
    } else {
        span *= 1.2;
    }
    if (!isfinite(span) || span <= 0.0) return CALC_RANGE_ERROR;
    model->y_center = (y_min + y_max) * 0.5;
    model->y_span = span;
    graph_model_clear_analysis(model);
    return CALC_OK;
}

static bool refine_crossing(graph_evaluate_fn evaluate, void *context,
                            size_t first_function, size_t second_function,
                            double left, double right,
                            double *root, double *root_y) {
    double left_first = 0.0;
    double left_second = 0.0;
    if (!evaluate(context, first_function, left, &left_first)) return false;
    if (second_function < GRAPH_FUNCTION_COUNT &&
        !evaluate(context, second_function, left, &left_second)) return false;
    double left_value = left_first - left_second;

    for (int iteration = 0; iteration < GRAPH_REFINE_STEPS; ++iteration) {
        double middle = (left + right) * 0.5;
        double middle_first = 0.0;
        double middle_second = 0.0;
        if (!evaluate(context, first_function, middle, &middle_first)) {
            return false;
        }
        if (second_function < GRAPH_FUNCTION_COUNT &&
            !evaluate(context, second_function, middle, &middle_second)) {
            return false;
        }
        double middle_value = middle_first - middle_second;
        if (fabs(middle_value) < 1e-10) {
            left = middle;
            right = middle;
            break;
        }
        if ((left_value < 0.0 && middle_value > 0.0) ||
            (left_value > 0.0 && middle_value < 0.0)) {
            right = middle;
        } else {
            left = middle;
            left_value = middle_value;
        }
    }

    *root = (left + right) * 0.5;
    if (!evaluate(context, first_function, *root, root_y)) return false;
    return isfinite(*root_y);
}

static bool append_marker(graph_marker_t *markers, size_t capacity,
                          size_t *count, const graph_marker_t *marker,
                          double minimum_distance) {
    for (size_t i = 0; i < *count; ++i) {
        if (fabs(markers[i].x - marker->x) < minimum_distance &&
            markers[i].first_function == marker->first_function &&
            markers[i].second_function == marker->second_function) {
            return false;
        }
    }
    if (*count >= capacity) return false;
    markers[*count] = *marker;
    (*count)++;
    return true;
}

static size_t find_crossings(const graph_model_t *model,
                             graph_evaluate_fn evaluate, void *context,
                             size_t first_function, size_t second_function,
                             graph_marker_t *markers, size_t capacity,
                             size_t count) {
    double x_min = model->x_center - model->x_span * 0.5;
    double step = model->x_span / GRAPH_SCAN_STEPS;
    double previous_x = x_min;
    double previous_first = 0.0;
    double previous_second = 0.0;
    bool previous_valid = evaluate(context, first_function, previous_x,
                                   &previous_first);
    if (second_function < GRAPH_FUNCTION_COUNT) {
        previous_valid = previous_valid &&
            evaluate(context, second_function, previous_x, &previous_second);
    }
    double previous_value = previous_first - previous_second;

    for (int sample = 1; sample <= GRAPH_SCAN_STEPS && count < capacity;
         ++sample) {
        double x = x_min + sample * step;
        double first = 0.0;
        double second = 0.0;
        bool valid = evaluate(context, first_function, x, &first);
        if (second_function < GRAPH_FUNCTION_COUNT) {
            valid = valid && evaluate(context, second_function, x, &second);
        }
        double value = first - second;
        if (valid && previous_valid) {
            graph_marker_t marker = {
                .first_function = first_function,
                .second_function = second_function,
            };
            bool found = false;
            bool previous_zero = fabs(previous_value) < 1e-12;
            bool current_zero = fabs(value) < 1e-12;
            if (previous_zero && current_zero) {
                found = false;
            } else if (previous_zero) {
                marker.x = previous_x;
                found = evaluate(context, first_function,
                                 marker.x, &marker.y);
            } else if (current_zero) {
                marker.x = x;
                marker.y = first;
                found = true;
            } else if ((value < 0.0) != (previous_value < 0.0)) {
                found = refine_crossing(evaluate, context, first_function,
                                        second_function, previous_x, x,
                                        &marker.x, &marker.y);
            }
            if (found) {
                append_marker(markers, capacity, &count, &marker, step * 0.5);
            }
        }
        previous_x = x;
        previous_value = value;
        previous_valid = valid;
    }
    return count;
}

size_t graph_model_find_roots(const graph_model_t *model,
                              graph_evaluate_fn evaluate, void *context,
                              graph_marker_t *markers, size_t capacity) {
    if (!evaluate || !markers) return 0;
    size_t count = 0;
    for (size_t function = 0;
         function < GRAPH_FUNCTION_COUNT && count < capacity; ++function) {
        if (!model->functions[function].active) continue;
        count = find_crossings(model, evaluate, context, function,
                               GRAPH_FUNCTION_COUNT, markers, capacity, count);
    }
    return count;
}

size_t graph_model_find_intersections(const graph_model_t *model,
                                      graph_evaluate_fn evaluate,
                                      void *context,
                                      graph_marker_t *markers,
                                      size_t capacity) {
    if (!evaluate || !markers) return 0;
    size_t count = 0;
    for (size_t first = 0; first < GRAPH_FUNCTION_COUNT; ++first) {
        if (!model->functions[first].active) continue;
        for (size_t second = first + 1;
             second < GRAPH_FUNCTION_COUNT && count < capacity; ++second) {
            if (!model->functions[second].active) continue;
            count = find_crossings(model, evaluate, context, first, second,
                                   markers, capacity, count);
        }
    }
    return count;
}
