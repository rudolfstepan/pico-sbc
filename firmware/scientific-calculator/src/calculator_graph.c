#include "calculator_graph.h"

#include "calculator_engine.h"
#include "calculator_keymaps.h"
#include "lcd_st7796.h"

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
#define GRAPH_PLOT_BOTTOM (CALCULATOR_KEY_Y + \
    4 * (CALCULATOR_KEY_HEIGHT + CALCULATOR_KEY_GAP_Y) - 4)
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
        evaluation->compiled[i] = calc_engine_compile_x(
            graph->functions[i].expression, ans, &error_position);
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
                                          double ans) {
    bool previous_degrees = calc_engine_uses_degrees();
    calc_engine_set_degrees(false);
    graph_evaluation_t evaluation;
    char ignored_message[24];
    size_t count = compile_functions(graph, ans, &evaluation,
                                     ignored_message, sizeof ignored_message);
    calc_status_t status = count
        ? graph_model_auto_scale(graph, graph_evaluate, &evaluation)
        : CALC_EMPTY;
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
    return (int)(((value - x_min) * (LCD_WIDTH - 1) / x_span) + 0.5);
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
        lcd_fill_rect(0, pixel_y, LCD_WIDTH, 1, COL_GRID);
    }
    if (x_min <= 0.0 && x_max >= 0.0) {
        int axis_x = graph_x_pixel(0.0, x_min, x_span);
        lcd_fill_rect(axis_x, GRAPH_PLOT_TOP, 1, plot_height, COL_MUTED);
    }
    if (y_min <= 0.0 && y_max >= 0.0) {
        int axis_y = graph_y_pixel(0.0, y_max, y_span,
                                   GRAPH_PLOT_TOP, plot_height);
        lcd_fill_rect(0, axis_y, LCD_WIDTH, 1, COL_MUTED);
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
        if (x + width >= LCD_WIDTH) x = LCD_WIDTH - width - 1;
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
        if (x + width >= LCD_WIDTH) x = axis_x - width - 3;
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
        lcd_draw_text(LCD_WIDTH - 8, y, "X", COL_TEXT, COL_BG, 1);
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
    for (int pixel_x = 0; pixel_x < LCD_WIDTH; ++pixel_x) {
        double x = x_min + graph->x_span * pixel_x / (LCD_WIDTH - 1);
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
    lcd_fill_rect(x - 3, y, 7, 1, color);
    lcd_fill_rect(x, y - 3, 1, 7, color);
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
    lcd_fill_rect(x - 3, pixel_y - 3, 7, 7,
                  function_colors[graph->selected_function]);
    lcd_fill_rect(x - 1, pixel_y - 1, 3, 3, COL_BG);
}

static void render_plot(const calculator_graph_t *graph,
                        graph_evaluation_t *evaluation) {
    double x_min = graph->x_center - graph->x_span * 0.5;
    double y_min = graph->y_center - graph->y_span * 0.5;
    double y_max = graph->y_center + graph->y_span * 0.5;
    int plot_height = GRAPH_PLOT_BOTTOM - GRAPH_PLOT_TOP;
    double x_step = graph_grid_step(graph->x_span, LCD_WIDTH, 56);
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
    if (trace_valid) {
        snprintf(status, sizeof status, "TRACE F%u  X %.6g  Y %.6g  RAD",
                 (unsigned int)(graph->selected_function + 1),
                 graph->trace_x, trace_y);
    } else {
        snprintf(status, sizeof status, "X %.4g..%.4g  Y %.4g..%.4g  RAD",
                 x_min, x_min + graph->x_span, y_min, y_max);
    }
    lcd_fill_rect(0, 0, LCD_WIDTH, GRAPH_PLOT_TOP, COL_BG);
    lcd_draw_text(4, 2, status, COL_TEXT, COL_BG, 1);

    char selected[84];
    snprintf(selected, sizeof selected, "F%u %s%.75s",
             (unsigned int)(graph->selected_function + 1),
             graph->functions[graph->selected_function].active ? "" : "OFF ",
             graph->functions[graph->selected_function].expression[0]
                ? graph->functions[graph->selected_function].expression
                : "EMPTY - EDIT IN TOOLS");
    lcd_draw_text(4, 15, calculator_widget_tail(selected, 76),
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
    static const int columns[4] = {4, 104, 224, 344};
    lcd_fill(COL_BG);
    char status[64];
    snprintf(status, sizeof status, "VALUE TABLE  X0 %.7g  STEP %.7g  RAD",
             graph->table_x, graph->table_step);
    lcd_draw_text(4, 2, status, COL_TEXT, COL_BG, 1);
    lcd_draw_text(columns[0], 18, "X", COL_MUTED, COL_BG, 1);
    for (size_t function = 0; function < GRAPH_FUNCTION_COUNT; ++function) {
        char label[4];
        snprintf(label, sizeof label, "F%u", (unsigned int)(function + 1));
        uint16_t color = graph->functions[function].active
            ? function_colors[function] : COL_MUTED;
        lcd_draw_text(columns[function + 1], 18, label, color, COL_BG, 1);
    }
    lcd_fill_rect(0, 28, LCD_WIDTH, 1, COL_GRID);

    for (int row = 0; row < 15; ++row) {
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
                             const calculator_widget_state_t *widget_state,
                             char *message, size_t message_size) {
    bool previous_degrees = calc_engine_uses_degrees();
    calc_engine_set_degrees(false);

    graph_evaluation_t evaluation;
    message[0] = '\0';
    size_t compiled_count = compile_functions(graph, ans, &evaluation,
                                              message, message_size);
    if (graph->view == GRAPH_VIEW_TABLE) {
        render_table(graph, &evaluation);
    } else {
        render_plot(graph, &evaluation);
    }

    if (!compiled_count) {
        snprintf(message, message_size, "SET FUNCTION IN TOOLS");
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
