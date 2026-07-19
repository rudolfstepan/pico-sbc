#include "calculator_graph.h"

#include "calculator_engine.h"
#include "calculator_keymaps.h"
#include "lcd_st7796.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define COL_BG    RGB565(0, 0, 0)
#define COL_TEXT  RGB565(255, 255, 255)
#define COL_MUTED RGB565(170, 170, 170)
#define COL_GRID  RGB565(48, 48, 48)

void calculator_graph_init(calculator_graph_t *graph) {
    graph->x_center = 0.0;
    graph->y_center = 0.0;
    graph->x_span = 20.0;
    graph->y_span = 10.0;
}

void calculator_graph_pan(calculator_graph_t *graph,
                          double horizontal_fraction,
                          double vertical_fraction) {
    graph->x_center += graph->x_span * horizontal_fraction;
    graph->y_center += graph->y_span * vertical_fraction;
}

void calculator_graph_zoom(calculator_graph_t *graph, double factor) {
    if (!(factor > 0.0) || !isfinite(factor)) return;
    graph->x_span *= factor;
    graph->y_span *= factor;
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
                            int plot_top, int plot_bottom,
                            double x_step, double y_step) {
    int plot_height = plot_bottom - plot_top;
    double x_max = x_min + x_span;
    double y_max = y_min + y_span;
    double first_x = ceil(x_min / x_step) * x_step;
    double first_y = ceil(y_min / y_step) * y_step;

    for (int tick = 0; tick < 64; ++tick) {
        double value = first_x + tick * x_step;
        if (value > x_max + x_step * 0.001) break;
        int pixel_x = graph_x_pixel(value, x_min, x_span);
        lcd_fill_rect(pixel_x, plot_top, 1, plot_height, COL_GRID);
    }
    for (int tick = 0; tick < 64; ++tick) {
        double value = first_y + tick * y_step;
        if (value > y_max + y_step * 0.001) break;
        int pixel_y = graph_y_pixel(value, y_max, y_span,
                                    plot_top, plot_height);
        lcd_fill_rect(0, pixel_y, LCD_WIDTH, 1, COL_GRID);
    }

    if (x_min <= 0.0 && x_max >= 0.0) {
        int axis_x = graph_x_pixel(0.0, x_min, x_span);
        lcd_fill_rect(axis_x, plot_top, 1, plot_height, COL_MUTED);
    }
    if (y_min <= 0.0 && y_max >= 0.0) {
        int axis_y = graph_y_pixel(0.0, y_max, y_span,
                                   plot_top, plot_height);
        lcd_fill_rect(0, axis_y, LCD_WIDTH, 1, COL_MUTED);
    }
}

static void draw_graph_axis_labels(double x_min, double x_span,
                                   double y_min, double y_span,
                                   int plot_top, int plot_bottom,
                                   double x_step, double y_step) {
    int plot_height = plot_bottom - plot_top;
    double x_max = x_min + x_span;
    double y_max = y_min + y_span;
    bool x_axis_visible = y_min <= 0.0 && y_max >= 0.0;
    bool y_axis_visible = x_min <= 0.0 && x_max >= 0.0;
    int axis_y = x_axis_visible
        ? graph_y_pixel(0.0, y_max, y_span, plot_top, plot_height)
        : plot_bottom - 1;
    int axis_x = y_axis_visible
        ? graph_x_pixel(0.0, x_min, x_span) : 0;
    int x_label_y = x_axis_visible ? axis_y + 3 : plot_bottom - 9;

    if (x_label_y + 8 > plot_bottom) x_label_y = axis_y - 10;
    if (x_label_y < plot_top) x_label_y = plot_top;

    double first_x = ceil(x_min / x_step) * x_step;
    for (int tick = 0; tick < 64; ++tick) {
        double value = first_x + tick * x_step;
        if (value > x_max + x_step * 0.001) break;

        char label[16];
        format_graph_tick(label, sizeof label, value, x_step);
        int label_width = (int)strlen(label) * 6;
        int label_x = graph_x_pixel(value, x_min, x_span) - label_width / 2;
        if (label_x < 1) label_x = 1;
        if (label_x + label_width >= LCD_WIDTH) {
            label_x = LCD_WIDTH - label_width - 1;
        }
        lcd_draw_text(label_x, x_label_y, label, COL_MUTED, COL_BG, 1);
    }

    double first_y = ceil(y_min / y_step) * y_step;
    for (int tick = 0; tick < 64; ++tick) {
        double value = first_y + tick * y_step;
        if (value > y_max + y_step * 0.001) break;
        if (x_axis_visible && fabs(value) < y_step * 0.0001) continue;

        char label[16];
        format_graph_tick(label, sizeof label, value, y_step);
        int label_width = (int)strlen(label) * 6;
        int label_x = 2;
        if (y_axis_visible) {
            label_x = axis_x + 4;
            if (label_x + label_width >= LCD_WIDTH) {
                label_x = axis_x - label_width - 3;
            }
        }
        int label_y = graph_y_pixel(value, y_max, y_span,
                                    plot_top, plot_height) - 4;
        if (label_y < plot_top) label_y = plot_top;
        if (label_y + 8 > plot_bottom) label_y = plot_bottom - 8;
        lcd_draw_text(label_x, label_y, label, COL_MUTED, COL_BG, 1);
    }

    if (x_axis_visible) {
        int name_y = x_label_y > axis_y ? axis_y - 10 : axis_y + 3;
        if (name_y < plot_top) name_y = plot_top;
        if (name_y + 8 > plot_bottom) name_y = plot_bottom - 8;
        lcd_draw_text(LCD_WIDTH - 8, name_y, "X", COL_TEXT, COL_BG, 1);
    }
    if (y_axis_visible) {
        int name_x = axis_x >= 9 ? axis_x - 8 : axis_x + 4;
        lcd_draw_text(name_x, plot_top + 1, "Y", COL_TEXT, COL_BG, 1);
    }
}

void calculator_graph_render(const calculator_graph_t *graph,
                             const char *expression, double ans,
                             const calculator_widget_state_t *widget_state,
                             char *message, size_t message_size) {
    const int plot_top = 18;
    const int plot_bottom = CALCULATOR_KEY_Y +
        4 * (CALCULATOR_KEY_HEIGHT + CALCULATOR_KEY_GAP_Y) - 4;
    const int plot_height = plot_bottom - plot_top;
    double x_min = graph->x_center - graph->x_span * 0.5;
    double y_min = graph->y_center - graph->y_span * 0.5;
    double y_max = graph->y_center + graph->y_span * 0.5;
    double x_step = graph_grid_step(graph->x_span, LCD_WIDTH, 56);
    double y_step = graph_grid_step(graph->y_span, plot_height, 44);
    int error_position = 0;

    lcd_fill(COL_BG);
    draw_graph_grid(x_min, graph->x_span, y_min, graph->y_span,
                    plot_top, plot_bottom, x_step, y_step);

    calc_compiled_t *compiled = calc_engine_compile_x(expression, ans,
                                                       &error_position);
    bool previous_valid = false;
    int previous_y = 0;
    if (compiled) {
        for (int pixel_x = 0; pixel_x < LCD_WIDTH; ++pixel_x) {
            double x = x_min + graph->x_span * pixel_x / (LCD_WIDTH - 1);
            double y = 0.0;
            bool valid = calc_engine_evaluate_x(compiled, x, &y) &&
                         y >= y_min && y <= y_max;
            if (valid) {
                int pixel_y = plot_top + (int)((y_max - y) *
                    (plot_height - 1) / graph->y_span);
                if (previous_valid &&
                    (pixel_y > previous_y ? pixel_y - previous_y
                                          : previous_y - pixel_y) <
                        plot_height / 2) {
                    int top = pixel_y < previous_y ? pixel_y : previous_y;
                    int height = pixel_y > previous_y
                        ? pixel_y - previous_y + 1 : previous_y - pixel_y + 1;
                    lcd_fill_rect(pixel_x, top, 1, height, COL_TEXT);
                } else {
                    lcd_fill_rect(pixel_x, pixel_y, 1, 1, COL_TEXT);
                }
                previous_y = pixel_y;
            }
            previous_valid = valid;
        }
        calc_engine_free(compiled);
        snprintf(message, message_size, "GRAPH READY");
    } else {
        snprintf(message, message_size, "GRAPH SYNTAX %d", error_position);
    }

    draw_graph_axis_labels(x_min, graph->x_span, y_min, graph->y_span,
                           plot_top, plot_bottom, x_step, y_step);

    char status[80];
    snprintf(status, sizeof status, "GRAPH X %.4g..%.4g  Y %.4g..%.4g",
             x_min, x_min + graph->x_span, y_min, y_max);
    lcd_fill_rect(0, 0, LCD_WIDTH, 16, COL_BG);
    lcd_draw_text(6, 4, status, COL_TEXT, COL_BG, 1);
    size_t key_count;
    const calc_key_t *keys = calculator_keymap(PAGE_GRAPH, &key_count);
    for (size_t i = 0; i < key_count; ++i) {
        calculator_widget_draw_key(&keys[i], false, widget_state);
    }
}
