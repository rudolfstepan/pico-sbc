#include "calculator_graph.h"

#include "calculator_engine.h"
#include "calculator_keymaps.h"
#include "lcd_st7796.h"
#include "numerical_analysis.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COL_BG       RGB565(0, 0, 0)
#define COL_TEXT     RGB565(255, 255, 255)
#define COL_MUTED    RGB565(170, 170, 170)
#define COL_GRID     RGB565(48, 48, 48)
#define COL_GRAPH_F1 RGB565(0, 220, 255)
#define COL_GRAPH_F2 RGB565(255, 220, 0)
#define COL_GRAPH_F3 RGB565(255, 80, 190)

#define GRAPH_PLOT_TOP 28
#define GRAPH_PLOT_BOTTOM (calculator_widget_key_top(4) - 4)
#define GRAPH_MARKER_CAPACITY 18

typedef struct {
    calc_compiled_t *compiled[GRAPH_FUNCTION_COUNT];
} graph_evaluation_t;

static const uint16_t function_colors[GRAPH_FUNCTION_COUNT] = {
    COL_GRAPH_F1, COL_GRAPH_F2, COL_GRAPH_F3
};

void calculator_graph_init(calculator_graph_t *graph) {
    graph_model_init(graph);
    graph_model_reset_range(graph);
}

void calculator_graph_pan(calculator_graph_t *graph,
                          double horizontal_fraction,
                          double vertical_fraction) {
    graph_model_pan(graph, horizontal_fraction, vertical_fraction);
}

void calculator_graph_zoom(calculator_graph_t *graph, double factor) {
    graph_model_zoom(graph, factor);
}

static bool graph_evaluate(void *context, size_t function_index,
                           double x, double *y) {
    graph_evaluation_t *evaluation = context;
    if (function_index >= GRAPH_FUNCTION_COUNT ||
        !evaluation->compiled[function_index]) {
        return false;
    }
    return calc_engine_evaluate_x(evaluation->compiled[function_index], x, y);
}

static size_t compile_functions(const calculator_graph_t *graph, double ans,
                                const calculator_symbols_t *symbols,
                                graph_evaluation_t *evaluation,
                                char *message, size_t message_size) {
    memset(evaluation, 0, sizeof *evaluation);
    size_t compiled_count = 0;
    for (size_t i = 0; i < GRAPH_FUNCTION_COUNT; ++i) {
        if (!graph->functions[i].active ||
            !graph->functions[i].expression[0]) {
            continue;
        }
        int error_position = 0;
        evaluation->compiled[i] = calc_engine_compile_x_symbols(
            graph->functions[i].expression, ans, symbols, &error_position);
        if (evaluation->compiled[i]) {
            compiled_count++;
        } else {
            snprintf(message, message_size, "F%u SYNTAX %d",
                     (unsigned int)(i + 1), error_position);
        }
    }
    return compiled_count;
}

static void free_functions(graph_evaluation_t *evaluation) {
    for (size_t i = 0; i < GRAPH_FUNCTION_COUNT; ++i) {
        calc_engine_free(evaluation->compiled[i]);
        evaluation->compiled[i] = NULL;
    }
}

calc_status_t calculator_graph_auto_scale(calculator_graph_t *graph,
                                          double ans,
                                          const calculator_symbols_t *symbols) {
    bool previous_degrees = calc_engine_uses_degrees();
    calc_engine_set_degrees(false);
    graph_evaluation_t evaluation;
    char ignored_message[24];
    size_t count = compile_functions(graph, ans, symbols, &evaluation,
                                     ignored_message, sizeof ignored_message);
    calc_status_t status = count
        ? graph_model_auto_scale(graph, graph_evaluate, &evaluation)
        : CALC_EMPTY;
    free_functions(&evaluation);
    calc_engine_set_degrees(previous_degrees);
    return status;
}

typedef struct {
    graph_evaluation_t *evaluation;
    size_t function;
} graph_slot_evaluation_t;

static bool evaluate_slot(void *context, double x, double *y) {
    graph_slot_evaluation_t *slot = context;
    return graph_evaluate(slot->evaluation, slot->function, x, y);
}

typedef struct {
    graph_slot_evaluation_t first;
    graph_slot_evaluation_t second;
} graph_difference_evaluation_t;

static bool evaluate_slot_difference(void *context, double x, double *y) {
    graph_difference_evaluation_t *difference = context;
    double first = 0.0;
    double second = 0.0;
    if (!evaluate_slot(&difference->first, x, &first) ||
        !evaluate_slot(&difference->second, x, &second)) {
        return false;
    }
    *y = first - second;
    return isfinite(*y);
}

static numerical_result_t root_near_guess(numerical_function_t function,
                                           void *context,
                                           double left, double right,
                                           double guess,
                                           numerical_options_t options) {
    numerical_result_t result = numerical_root_guess(
        function, context, guess, options);
    if (result.status == CALC_OK && result.x >= left && result.x <= right) {
        return result;
    }

    numerical_result_t best = result;
    best.status = CALC_CONVERGENCE_ERROR;
    double best_distance = INFINITY;
    double previous_x = left;
    double previous_value = 0.0;
    bool previous_valid = function(context, previous_x, &previous_value) &&
                          isfinite(previous_value);
    for (unsigned int sample = 1; sample <= options.sample_count; ++sample) {
        double x = left + (right - left) * sample / options.sample_count;
        double value = 0.0;
        bool valid = function(context, x, &value) && isfinite(value);
        if (valid && previous_valid &&
            (fabs(value) <= options.tolerance ||
             fabs(previous_value) <= options.tolerance ||
             (value < 0.0) != (previous_value < 0.0))) {
            numerical_result_t candidate = numerical_root_interval(
                function, context, previous_x, x, options);
            if (candidate.status == CALC_OK) {
                double distance = fabs(candidate.x - guess);
                if (distance < best_distance) {
                    best = candidate;
                    best_distance = distance;
                }
            }
        }
        previous_x = x;
        previous_value = value;
        previous_valid = valid;
    }
    return best;
}

static size_t next_active_function(const calculator_graph_t *graph,
                                   const graph_evaluation_t *evaluation) {
    for (size_t offset = 1; offset < GRAPH_FUNCTION_COUNT; ++offset) {
        size_t function = (graph->selected_function + offset) %
                          GRAPH_FUNCTION_COUNT;
        if (evaluation->compiled[function]) return function;
    }
    return GRAPH_FUNCTION_COUNT;
}

static const char *analysis_name(calculator_graph_analysis_t analysis) {
    switch (analysis) {
        case CALCULATOR_GRAPH_ROOT: return "ROOT";
        case CALCULATOR_GRAPH_INTERSECTION: return "XING";
        case CALCULATOR_GRAPH_DERIVATIVE: return "DERIV";
        case CALCULATOR_GRAPH_INTEGRAL: return "INTEGR";
        case CALCULATOR_GRAPH_EXTREMA: return "EXTREM";
        default: return "ANALYSIS";
    }
}

static void set_analysis_error(calculator_graph_t *graph,
                               calculator_graph_analysis_t analysis,
                               calc_status_t status,
                               numerical_options_t options) {
    snprintf(graph->analysis_text, sizeof graph->analysis_text,
             "%s %s  TOL %.0e  MAX %u",
             analysis_name(analysis), calculation_status_text(status),
             options.tolerance, options.max_iterations);
}

calc_status_t calculator_graph_analyze(calculator_graph_t *graph, double ans,
                                       const calculator_symbols_t *symbols,
                                       calculator_graph_analysis_t analysis) {
    numerical_options_t options = numerical_default_options();
    options.tolerance = graph->analysis_tolerance;
    options.max_iterations = graph->analysis_max_iterations;
    bool previous_degrees = calc_engine_uses_degrees();
    calc_engine_set_degrees(false);
    graph_evaluation_t evaluation;
    char compile_message[24] = "";
    compile_functions(graph, ans, symbols, &evaluation,
                      compile_message, sizeof compile_message);

    size_t selected = graph->selected_function;
    calc_status_t status = CALC_EMPTY;
    if (!evaluation.compiled[selected]) {
        snprintf(graph->analysis_text, sizeof graph->analysis_text,
                 "F%u %s", (unsigned int)(selected + 1),
                 compile_message[0] ? compile_message : "IS EMPTY OR OFF");
        free_functions(&evaluation);
        calc_engine_set_degrees(previous_degrees);
        return status;
    }

    double left = 0.0;
    double right = 0.0;
    graph_model_analysis_interval(graph, &left, &right);
    graph_slot_evaluation_t slot = {
        .evaluation = &evaluation,
        .function = selected,
    };
    numerical_result_t result;

    switch (analysis) {
        case CALCULATOR_GRAPH_ROOT:
            result = root_near_guess(evaluate_slot, &slot, left, right,
                                     graph->trace_x, options);
            status = result.status;
            if (status == CALC_OK) {
                graph->trace_enabled = true;
                graph->trace_x = result.x;
                snprintf(graph->analysis_text, sizeof graph->analysis_text,
                         "ROOT F%u  X %.9g  I %u",
                         (unsigned int)(selected + 1), result.x,
                         result.iterations);
            }
            break;

        case CALCULATOR_GRAPH_INTERSECTION: {
            size_t second = next_active_function(graph, &evaluation);
            if (second >= GRAPH_FUNCTION_COUNT) {
                status = CALC_EMPTY;
                snprintf(graph->analysis_text, sizeof graph->analysis_text,
                         "XING NEED SECOND ACTIVE FUNCTION");
                break;
            }
            graph_difference_evaluation_t difference = {
                .first = slot,
                .second = {
                    .evaluation = &evaluation,
                    .function = second,
                },
            };
            result = root_near_guess(evaluate_slot_difference, &difference,
                                     left, right, graph->trace_x, options);
            status = result.status;
            if (status == CALC_OK &&
                evaluate_slot(&slot, result.x, &result.value)) {
                graph->trace_enabled = true;
                graph->trace_x = result.x;
                snprintf(graph->analysis_text, sizeof graph->analysis_text,
                         "XING F%u/F%u  X %.8g  Y %.8g  I %u",
                         (unsigned int)(selected + 1),
                         (unsigned int)(second + 1), result.x, result.value,
                         result.iterations);
            } else if (status == CALC_OK) {
                status = CALC_DOMAIN_ERROR;
            }
            break;
        }

        case CALCULATOR_GRAPH_DERIVATIVE:
            result = numerical_derivative(evaluate_slot, &slot,
                                          graph->trace_x, options);
            status = result.status;
            if (status == CALC_OK) {
                graph->trace_enabled = true;
                snprintf(graph->analysis_text, sizeof graph->analysis_text,
                         "DERIV F%u  X %.8g  D %.9g  I %u",
                         (unsigned int)(selected + 1), result.x, result.value,
                         result.iterations);
            }
            break;

        case CALCULATOR_GRAPH_INTEGRAL:
            result = numerical_integral(evaluate_slot, &slot,
                                        left, right, options);
            status = result.status;
            if (status == CALC_OK) {
                snprintf(graph->analysis_text, sizeof graph->analysis_text,
                         "INTEGR F%u  [%.5g,%.5g] = %.9g  I %u",
                         (unsigned int)(selected + 1), left, right,
                         result.value, result.iterations);
            }
            break;

        case CALCULATOR_GRAPH_EXTREMA: {
            numerical_extremum_t extrema[12];
            calc_status_t extrema_status = CALC_EMPTY;
            size_t count = numerical_find_extrema(
                evaluate_slot, &slot, left, right, options,
                extrema, sizeof extrema / sizeof extrema[0], &extrema_status);
            status = extrema_status;
            if (status == CALC_OK) {
                const numerical_extremum_t *minimum = NULL;
                const numerical_extremum_t *maximum = NULL;
                for (size_t i = 0; i < count; ++i) {
                    if (extrema[i].kind == NUMERICAL_MINIMUM && !minimum) {
                        minimum = &extrema[i];
                    }
                    if (extrema[i].kind == NUMERICAL_MAXIMUM && !maximum) {
                        maximum = &extrema[i];
                    }
                }
                if (minimum && maximum) {
                    graph->trace_enabled = true;
                    graph->trace_x = minimum->x;
                    snprintf(graph->analysis_text,
                             sizeof graph->analysis_text,
                             "EXT F%u N%u  MIN %.6g@%.6g  MAX %.6g@%.6g",
                             (unsigned int)(selected + 1),
                             (unsigned int)count,
                             minimum->value, minimum->x,
                             maximum->value, maximum->x);
                } else {
                    const numerical_extremum_t *extremum = minimum
                        ? minimum : maximum;
                    graph->trace_enabled = true;
                    graph->trace_x = extremum->x;
                    snprintf(graph->analysis_text,
                             sizeof graph->analysis_text,
                             "EXT F%u N%u  %s %.8g @ %.8g",
                             (unsigned int)(selected + 1),
                             (unsigned int)count,
                             minimum ? "MIN" : "MAX",
                             extremum->value, extremum->x);
                }
            }
            break;
        }
    }

    if (status != CALC_OK && !graph->analysis_text[0]) {
        set_analysis_error(graph, analysis, status, options);
    } else if (status != CALC_OK &&
               strncmp(graph->analysis_text, "XING NEED", 9) != 0) {
        set_analysis_error(graph, analysis, status, options);
    }
    free_functions(&evaluation);
    calc_engine_set_degrees(previous_degrees);
    return status;
}

static double graph_grid_step(double span, int pixels, int target_pixels) {
    double raw_step = span * target_pixels / pixels;
    if (!(raw_step > 0.0) || !isfinite(raw_step)) return 1.0;

    double magnitude = pow(10.0, floor(log10(raw_step)));
    if (!(magnitude > 0.0) || !isfinite(magnitude)) return raw_step;

    double normalized = raw_step / magnitude;
    double multiple = normalized < 1.5 ? 1.0 :
                      normalized < 3.5 ? 2.0 :
                      normalized < 7.5 ? 5.0 : 10.0;
    return multiple * magnitude;
}

static int graph_x_pixel(double value, double x_min, double x_span) {
    return (int)(((value - x_min) * (lcd_width() - 1) / x_span) + 0.5);
}

static int graph_y_pixel(double value, double y_max, double y_span,
                         int plot_top, int plot_height) {
    return plot_top +
           (int)(((y_max - value) * (plot_height - 1) / y_span) + 0.5);
}

static void format_graph_tick(char *buffer, size_t size, double value,
                              double step) {
    if (fabs(value) < step * 0.0001) value = 0.0;
    double absolute = fabs(value);
    if ((absolute > 0.0 && absolute < 0.001) || absolute >= 100000.0 ||
        step < 0.001 || step >= 100000.0) {
        snprintf(buffer, size, "%.2g", value);
        return;
    }
    int decimals = step < 1.0 ? (int)ceil(-log10(step)) : 0;
    if (decimals < 0) decimals = 0;
    if (decimals > 4) decimals = 4;
    snprintf(buffer, size, "%.*f", decimals, value);
}

static void draw_graph_grid(double x_min, double x_span,
                            double y_min, double y_span,
                            double x_step, double y_step) {
    int plot_height = GRAPH_PLOT_BOTTOM - GRAPH_PLOT_TOP;
    double x_max = x_min + x_span;
    double y_max = y_min + y_span;
    double first_x = ceil(x_min / x_step) * x_step;
    double first_y = ceil(y_min / y_step) * y_step;

    for (int tick = 0; tick < 64; ++tick) {
        double value = first_x + tick * x_step;
        if (value > x_max + x_step * 0.001) break;
        int pixel_x = graph_x_pixel(value, x_min, x_span);
        lcd_fill_rect(pixel_x, GRAPH_PLOT_TOP, 1, plot_height, COL_GRID);
    }
    for (int tick = 0; tick < 64; ++tick) {
        double value = first_y + tick * y_step;
        if (value > y_max + y_step * 0.001) break;
        int pixel_y = graph_y_pixel(value, y_max, y_span,
                                    GRAPH_PLOT_TOP, plot_height);
        lcd_fill_rect(0, pixel_y, lcd_width(), 1, COL_GRID);
    }
    if (x_min <= 0.0 && x_max >= 0.0) {
        int axis_x = graph_x_pixel(0.0, x_min, x_span);
        lcd_fill_rect(axis_x, GRAPH_PLOT_TOP, 1, plot_height, COL_MUTED);
    }
    if (y_min <= 0.0 && y_max >= 0.0) {
        int axis_y = graph_y_pixel(0.0, y_max, y_span,
                                   GRAPH_PLOT_TOP, plot_height);
        lcd_fill_rect(0, axis_y, lcd_width(), 1, COL_MUTED);
    }
}

static void draw_graph_axis_labels(double x_min, double x_span,
                                   double y_min, double y_span,
                                   double x_step, double y_step) {
    int plot_height = GRAPH_PLOT_BOTTOM - GRAPH_PLOT_TOP;
    double x_max = x_min + x_span;
    double y_max = y_min + y_span;
    bool x_axis_visible = y_min <= 0.0 && y_max >= 0.0;
    bool y_axis_visible = x_min <= 0.0 && x_max >= 0.0;
    int axis_y = x_axis_visible
        ? graph_y_pixel(0.0, y_max, y_span, GRAPH_PLOT_TOP, plot_height)
        : GRAPH_PLOT_BOTTOM - 1;
    int axis_x = y_axis_visible
        ? graph_x_pixel(0.0, x_min, x_span) : 0;
    int x_label_y = x_axis_visible ? axis_y + 3 : GRAPH_PLOT_BOTTOM - 9;
    if (x_label_y + 8 > GRAPH_PLOT_BOTTOM) x_label_y = axis_y - 10;
    if (x_label_y < GRAPH_PLOT_TOP) x_label_y = GRAPH_PLOT_TOP;

    double first_x = ceil(x_min / x_step) * x_step;
    for (int tick = 0; tick < 64; ++tick) {
        double value = first_x + tick * x_step;
        if (value > x_max + x_step * 0.001) break;
        char label[16];
        format_graph_tick(label, sizeof label, value, x_step);
        int width = (int)strlen(label) * 6;
        int x = graph_x_pixel(value, x_min, x_span) - width / 2;
        if (x < 1) x = 1;
        if (x + width >= lcd_width()) x = lcd_width() - width - 1;
        lcd_draw_text(x, x_label_y, label, COL_MUTED, COL_BG, 1);
    }

    double first_y = ceil(y_min / y_step) * y_step;
    for (int tick = 0; tick < 64; ++tick) {
        double value = first_y + tick * y_step;
        if (value > y_max + y_step * 0.001) break;
        if (x_axis_visible && fabs(value) < y_step * 0.0001) continue;
        char label[16];
        format_graph_tick(label, sizeof label, value, y_step);
        int width = (int)strlen(label) * 6;
        int x = y_axis_visible ? axis_x + 4 : 2;
        if (x + width >= lcd_width()) x = axis_x - width - 3;
        int y = graph_y_pixel(value, y_max, y_span,
                              GRAPH_PLOT_TOP, plot_height) - 4;
        if (y < GRAPH_PLOT_TOP) y = GRAPH_PLOT_TOP;
        if (y + 8 > GRAPH_PLOT_BOTTOM) y = GRAPH_PLOT_BOTTOM - 8;
        lcd_draw_text(x, y, label, COL_MUTED, COL_BG, 1);
    }

    if (x_axis_visible) {
        int y = x_label_y > axis_y ? axis_y - 10 : axis_y + 3;
        if (y < GRAPH_PLOT_TOP) y = GRAPH_PLOT_TOP;
        if (y + 8 > GRAPH_PLOT_BOTTOM) y = GRAPH_PLOT_BOTTOM - 8;
        lcd_draw_text(lcd_width() - 8, y, "X", COL_TEXT, COL_BG, 1);
    }
    if (y_axis_visible) {
        int x = axis_x >= 9 ? axis_x - 8 : axis_x + 4;
        lcd_draw_text(x, GRAPH_PLOT_TOP + 1, "Y", COL_TEXT, COL_BG, 1);
    }
}

static void draw_function(const calculator_graph_t *graph,
                          graph_evaluation_t *evaluation, size_t function,
                          double x_min, double y_min, double y_max) {
    if (!evaluation->compiled[function]) return;
    int plot_height = GRAPH_PLOT_BOTTOM - GRAPH_PLOT_TOP;
    bool previous_valid = false;
    int previous_y = 0;
    for (int pixel_x = 0; pixel_x < lcd_width(); ++pixel_x) {
        double x = x_min + graph->x_span * pixel_x / (lcd_width() - 1);
        double y = 0.0;
        bool valid = graph_evaluate(evaluation, function, x, &y) &&
                     y >= y_min && y <= y_max;
        if (valid) {
            int pixel_y = graph_y_pixel(y, y_max, graph->y_span,
                                        GRAPH_PLOT_TOP, plot_height);
            if (previous_valid && abs(pixel_y - previous_y) < plot_height / 2) {
                int top = pixel_y < previous_y ? pixel_y : previous_y;
                int height = abs(pixel_y - previous_y) + 1;
                lcd_fill_rect(pixel_x, top, 1, height,
                              function_colors[function]);
            } else {
                lcd_fill_rect(pixel_x, pixel_y, 1, 1,
                              function_colors[function]);
            }
            previous_y = pixel_y;
        }
        previous_valid = valid;
    }
}

static void draw_marker(const calculator_graph_t *graph,
                        const graph_marker_t *marker,
                        double x_min, double y_min, double y_max) {
    if (marker->x < x_min || marker->x > x_min + graph->x_span ||
        marker->y < y_min || marker->y > y_max) {
        return;
    }
    int plot_height = GRAPH_PLOT_BOTTOM - GRAPH_PLOT_TOP;
    int x = graph_x_pixel(marker->x, x_min, graph->x_span);
    int y = graph_y_pixel(marker->y, y_max, graph->y_span,
                          GRAPH_PLOT_TOP, plot_height);
    uint16_t color = marker->second_function < GRAPH_FUNCTION_COUNT
        ? COL_TEXT : function_colors[marker->first_function];
    int horizontal_left = x > 3 ? x - 3 : 0;
    int horizontal_right = x + 3 < lcd_width()
        ? x + 3 : lcd_width() - 1;
    int vertical_top = y > GRAPH_PLOT_TOP + 3 ? y - 3 : GRAPH_PLOT_TOP;
    int vertical_bottom = y + 3 < GRAPH_PLOT_BOTTOM
        ? y + 3 : GRAPH_PLOT_BOTTOM - 1;
    lcd_fill_rect(horizontal_left, y,
                  horizontal_right - horizontal_left + 1, 1, color);
    lcd_fill_rect(x, vertical_top, 1,
                  vertical_bottom - vertical_top + 1, color);
}

static void draw_trace(const calculator_graph_t *graph,
                       graph_evaluation_t *evaluation,
                       double x_min, double y_min, double y_max) {
    if (!graph->trace_enabled ||
        !evaluation->compiled[graph->selected_function]) {
        return;
    }
    double y = 0.0;
    if (!graph_evaluate(evaluation, graph->selected_function,
                        graph->trace_x, &y) || y < y_min || y > y_max) {
        return;
    }
    int plot_height = GRAPH_PLOT_BOTTOM - GRAPH_PLOT_TOP;
    int x = graph_x_pixel(graph->trace_x, x_min, graph->x_span);
    int pixel_y = graph_y_pixel(y, y_max, graph->y_span,
                                GRAPH_PLOT_TOP, plot_height);
    lcd_fill_rect(x, GRAPH_PLOT_TOP, 1, plot_height, COL_MUTED);
    int left = x > 3 ? x - 3 : 0;
    int right = x + 3 < lcd_width() ? x + 3 : lcd_width() - 1;
    int top = pixel_y > GRAPH_PLOT_TOP + 3
        ? pixel_y - 3 : GRAPH_PLOT_TOP;
    int bottom = pixel_y + 3 < GRAPH_PLOT_BOTTOM
        ? pixel_y + 3 : GRAPH_PLOT_BOTTOM - 1;
    lcd_fill_rect(left, top, right - left + 1, bottom - top + 1,
                  function_colors[graph->selected_function]);
    int inner_left = x > 1 ? x - 1 : 0;
    int inner_right = x + 1 < lcd_width() ? x + 1 : lcd_width() - 1;
    int inner_top = pixel_y > GRAPH_PLOT_TOP + 1
        ? pixel_y - 1 : GRAPH_PLOT_TOP;
    int inner_bottom = pixel_y + 1 < GRAPH_PLOT_BOTTOM
        ? pixel_y + 1 : GRAPH_PLOT_BOTTOM - 1;
    lcd_fill_rect(inner_left, inner_top,
                  inner_right - inner_left + 1,
                  inner_bottom - inner_top + 1, COL_BG);
}

static void render_plot(const calculator_graph_t *graph,
                        graph_evaluation_t *evaluation) {
    double x_min = graph->x_center - graph->x_span * 0.5;
    double y_min = graph->y_center - graph->y_span * 0.5;
    double y_max = graph->y_center + graph->y_span * 0.5;
    int plot_height = GRAPH_PLOT_BOTTOM - GRAPH_PLOT_TOP;
    double x_step = graph_grid_step(graph->x_span, lcd_width(), 56);
    double y_step = graph_grid_step(graph->y_span, plot_height, 44);

    lcd_fill(COL_BG);
    draw_graph_grid(x_min, graph->x_span, y_min, graph->y_span,
                    x_step, y_step);
    for (size_t i = 0; i < GRAPH_FUNCTION_COUNT; ++i) {
        draw_function(graph, evaluation, i, x_min, y_min, y_max);
    }
    draw_graph_axis_labels(x_min, graph->x_span, y_min, graph->y_span,
                           x_step, y_step);

    graph_marker_t markers[GRAPH_MARKER_CAPACITY];
    size_t marker_count = graph_model_find_roots(
        graph, graph_evaluate, evaluation, markers, GRAPH_MARKER_CAPACITY);
    marker_count += graph_model_find_intersections(
        graph, graph_evaluate, evaluation, markers + marker_count,
        GRAPH_MARKER_CAPACITY - marker_count);
    for (size_t i = 0; i < marker_count; ++i) {
        draw_marker(graph, &markers[i], x_min, y_min, y_max);
    }
    draw_trace(graph, evaluation, x_min, y_min, y_max);

    char status[80];
    double trace_y = 0.0;
    bool trace_valid = graph->trace_enabled &&
        graph_evaluate(evaluation, graph->selected_function,
                       graph->trace_x, &trace_y);
    if (graph->analysis_text[0]) {
        snprintf(status, sizeof status, "%.79s", graph->analysis_text);
    } else if (trace_valid) {
        snprintf(status, sizeof status, "TRACE F%u  X %.6g  Y %.6g  RAD",
                 (unsigned int)(graph->selected_function + 1),
                 graph->trace_x, trace_y);
    } else {
        snprintf(status, sizeof status, "X %.4g..%.4g  Y %.4g..%.4g  RAD",
                 x_min, x_min + graph->x_span, y_min, y_max);
    }
    lcd_fill_rect(0, 0, lcd_width(), GRAPH_PLOT_TOP, COL_BG);
    size_t status_chars = (size_t)(lcd_width() - 8) / 6u;
    if (status_chars < sizeof status) status[status_chars] = '\0';
    lcd_draw_text(4, 2, status, COL_TEXT, COL_BG, 1);

    char selected[84];
    if (graph->view == GRAPH_VIEW_ANALYSIS ||
        graph->view == GRAPH_VIEW_ANALYSIS_MORE) {
        double analysis_left = 0.0;
        double analysis_right = 0.0;
        graph_model_analysis_interval(graph, &analysis_left, &analysis_right);
        snprintf(selected, sizeof selected,
                 "F%u  TOL %.0e  MAX %u  %s %.5g..%.5g",
                 (unsigned int)(graph->selected_function + 1),
                 graph->analysis_tolerance,
                 graph->analysis_max_iterations,
                 graph->custom_analysis_interval ? "A/B" : "VIEW",
                 analysis_left, analysis_right);
    } else {
        snprintf(selected, sizeof selected, "F%u %s%.75s",
                 (unsigned int)(graph->selected_function + 1),
                 graph->functions[graph->selected_function].active
                    ? "" : "OFF ",
                 graph->functions[graph->selected_function].expression[0]
                    ? graph->functions[graph->selected_function].expression
                    : "EMPTY - EDIT IN TOOLS");
    }
    size_t selected_chars = (size_t)(lcd_width() - 8) / 6u;
    lcd_draw_text(4, 15, calculator_widget_tail(selected, selected_chars),
                  function_colors[graph->selected_function], COL_BG, 1);
}

static void format_table_value(char *buffer, size_t size,
                               bool valid, double value) {
    if (!valid || !isfinite(value)) {
        snprintf(buffer, size, "--");
    } else {
        snprintf(buffer, size, "%.7g", value);
    }
}

static void render_table(const calculator_graph_t *graph,
                         graph_evaluation_t *evaluation) {
    int column_width = lcd_width() / 4;
    int columns[4] = {4, column_width + 4,
                      column_width * 2 + 4, column_width * 3 + 4};
    lcd_fill(COL_BG);
    char status[64];
    snprintf(status, sizeof status, "VALUE TABLE  X0 %.7g  STEP %.7g  RAD",
             graph->table_x, graph->table_step);
    size_t status_chars = (size_t)(lcd_width() - 8) / 6u;
    if (status_chars < sizeof status) status[status_chars] = '\0';
    lcd_draw_text(4, 2, status, COL_TEXT, COL_BG, 1);
    lcd_draw_text(columns[0], 18, "X", COL_MUTED, COL_BG, 1);
    for (size_t function = 0; function < GRAPH_FUNCTION_COUNT; ++function) {
        char label[4];
        snprintf(label, sizeof label, "F%u", (unsigned int)(function + 1));
        uint16_t color = graph->functions[function].active
            ? function_colors[function] : COL_MUTED;
        lcd_draw_text(columns[function + 1], 18, label, color, COL_BG, 1);
    }
    lcd_fill_rect(0, 28, lcd_width(), 1, COL_GRID);

    int table_bottom = calculator_widget_key_top(4) - 4;
    int visible_rows = (table_bottom - 34) / 15;
    for (int row = 0; row < visible_rows; ++row) {
        double x = graph->table_x + row * graph->table_step;
        char value[20];
        format_table_value(value, sizeof value, true, x);
        int y = 34 + row * 15;
        lcd_draw_text(columns[0], y, value, COL_TEXT, COL_BG, 1);
        for (size_t function = 0; function < GRAPH_FUNCTION_COUNT; ++function) {
            double result = 0.0;
            bool valid = graph_evaluate(evaluation, function, x, &result);
            format_table_value(value, sizeof value, valid, result);
            lcd_draw_text(columns[function + 1], y, value,
                          valid ? function_colors[function] : COL_MUTED,
                          COL_BG, 1);
        }
    }
}

void calculator_graph_render(const calculator_graph_t *graph,
                             double ans,
                             const calculator_symbols_t *symbols,
                             const calculator_widget_state_t *widget_state,
                             char *message, size_t message_size) {
    bool previous_degrees = calc_engine_uses_degrees();
    calc_engine_set_degrees(false);

    graph_evaluation_t evaluation;
    message[0] = '\0';
    size_t compiled_count = compile_functions(graph, ans, symbols, &evaluation,
                                              message, message_size);
    if (graph->view == GRAPH_VIEW_TABLE) {
        render_table(graph, &evaluation);
    } else {
        render_plot(graph, &evaluation);
    }

    if (!compiled_count) {
        /* Keep a compile error (e.g. "F1 SYNTAX 3") visible instead of
         * claiming that no function is set. */
        if (!message[0]) {
            snprintf(message, message_size, "SET FUNCTION IN TOOLS");
        }
    } else if (!message[0]) {
        snprintf(message, message_size, "GRAPH READY");
    }

    size_t key_count;
    const calc_key_t *keys = calculator_graph_keymap(graph->view, &key_count);
    for (size_t i = 0; i < key_count; ++i) {
        calculator_widget_draw_key(&keys[i], false, widget_state);
    }

    free_functions(&evaluation);
    calc_engine_set_degrees(previous_degrees);
}
