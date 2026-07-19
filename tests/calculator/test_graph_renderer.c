#include "calculator_engine.h"
#include "calculator_graph.h"
#include "graph_model.h"
#include "lcd_st7796.h"
#include "mock_lcd.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define COL_GRAPH_F1 RGB565(0, 220, 255)
#define PLOT_TOP 28
#define PLOT_BOTTOM 268

static int failures;

static calculator_widget_state_t widget_state(const graph_model_t *graph) {
    calculator_widget_state_t state = {
        .page = PAGE_GRAPH,
        .degrees = true,
        .graph_view = graph->view,
        .graph_active_mask = 1u,
        .graph_selected_function = graph->selected_function,
    };
    return state;
}

static void expect_render_in_bounds(graph_model_t *graph,
                                    const calculator_symbols_t *symbols,
                                    graph_view_t view) {
    char message[24] = "READY";
    graph_model_set_view(graph, view);
    calculator_widget_state_t state = widget_state(graph);
    mock_lcd_reset();
    calculator_graph_render(graph, 0.0, symbols, &state, message,
                            sizeof message);
    if (mock_lcd_had_out_of_bounds_draw()) {
        printf("FAIL: out-of-bounds drawing in graph view %d\n", view);
        failures++;
    }
    if (strcmp(message, "GRAPH READY") != 0) {
        printf("FAIL: graph message in view %d: %s\n", view, message);
        failures++;
    }
}

int main(void) {
    graph_model_t graph;
    calculator_symbols_t symbols;
    calculator_symbols_init(&symbols);
    calculator_graph_init(&graph);
    graph_model_set_function(&graph, 0, "sin(x)");

    calc_engine_set_degrees(true);
    if (calculator_graph_auto_scale(&graph, 0.0, &symbols) != CALC_OK) {
        printf("FAIL: sine auto scale\n");
        failures++;
    }
    if (!calc_engine_uses_degrees()) {
        printf("FAIL: auto scale did not restore DEG mode\n");
        failures++;
    }
    if (graph.y_span < 2.0 || graph.y_span > 2.5) {
        printf("FAIL: sine y span %.12g\n", graph.y_span);
        failures++;
    }

    calculator_symbols_set_variable(&symbols, 0, 2.0);
    calculator_symbols_set_function(&symbols, 0, "x+A");
    graph_model_set_function(&graph, 0, "f1(x)");
    calc_compiled_t *symbol_graph = calc_engine_compile_x_symbols(
        graph.functions[0].expression, 0.0, &symbols, NULL);
    double symbol_value = 0.0;
    if (!symbol_graph ||
        !calc_engine_evaluate_x(symbol_graph, 3.0, &symbol_value) ||
        fabs(symbol_value - 5.0) > 1e-12) {
        printf("FAIL: graph does not use shared symbols, value=%.12g\n",
               symbol_value);
        failures++;
    }
    calc_engine_free(symbol_graph);
    calculator_symbols_set_function(&symbols, 0, "");
    graph_model_set_function(&graph, 0, "sin(x)");

    char message[24] = "READY";
    calculator_widget_state_t state = widget_state(&graph);
    mock_lcd_reset();
    calculator_graph_render(&graph, 0.0, &symbols, &state, message,
                            sizeof message);
    if (!calc_engine_uses_degrees()) {
        printf("FAIL: render did not restore DEG mode\n");
        failures++;
    }

    int minimum_y = PLOT_BOTTOM;
    int maximum_y = PLOT_TOP;
    int occupied_columns = 0;
    for (int x = 0; x < LCD_WIDTH; ++x) {
        bool occupied = false;
        for (int y = PLOT_TOP; y < PLOT_BOTTOM; ++y) {
            if (mock_lcd_pixel(x, y) == COL_GRAPH_F1) {
                if (y < minimum_y) minimum_y = y;
                if (y > maximum_y) maximum_y = y;
                occupied = true;
            }
        }
        if (occupied) occupied_columns++;
    }
    if (maximum_y - minimum_y < 150 || occupied_columns < 450) {
        printf("FAIL: sine appears flat, dy=%d columns=%d\n",
               maximum_y - minimum_y, occupied_columns);
        failures++;
    }
    if (mock_lcd_had_out_of_bounds_draw()) {
        printf("FAIL: out-of-bounds plot drawing\n");
        failures++;
    }

    if (calculator_graph_analyze(&graph, 0.0, &symbols,
                                 CALCULATOR_GRAPH_ROOT) !=
            CALC_OK ||
        strstr(graph.analysis_text, "ROOT F1") == NULL ||
        fabs(graph.trace_x) > 1e-8) {
        printf("FAIL: graph root analysis: %s\n", graph.analysis_text);
        failures++;
    }
    graph.trace_x = 0.0;
    if (calculator_graph_analyze(&graph, 0.0, &symbols,
                                 CALCULATOR_GRAPH_DERIVATIVE) != CALC_OK ||
        strstr(graph.analysis_text, "DERIV F1") == NULL) {
        printf("FAIL: graph derivative analysis: %s\n", graph.analysis_text);
        failures++;
    }
    if (calculator_graph_analyze(&graph, 0.0, &symbols,
                                 CALCULATOR_GRAPH_INTEGRAL) != CALC_OK ||
        strstr(graph.analysis_text, "INTEGR F1") == NULL) {
        printf("FAIL: graph integral analysis: %s\n", graph.analysis_text);
        failures++;
    }
    graph_model_set_analysis_bound(&graph, true, 0.0);
    graph_model_set_analysis_bound(&graph, false, 3.14159265358979323846);
    if (calculator_graph_analyze(&graph, 0.0, &symbols,
                                 CALCULATOR_GRAPH_INTEGRAL) != CALC_OK ||
        strstr(graph.analysis_text, "= 2") == NULL) {
        printf("FAIL: custom interval integral: %s\n", graph.analysis_text);
        failures++;
    }
    graph_model_use_view_interval(&graph);
    if (calculator_graph_analyze(&graph, 0.0, &symbols,
                                 CALCULATOR_GRAPH_EXTREMA) != CALC_OK ||
        strstr(graph.analysis_text, "EXT F1") == NULL) {
        printf("FAIL: graph extrema analysis: %s\n", graph.analysis_text);
        failures++;
    }
    graph_model_set_function(&graph, 1, "0");
    graph_model_select_function(&graph, 0);
    graph.trace_x = 0.0;
    if (calculator_graph_analyze(&graph, 0.0, &symbols,
                                 CALCULATOR_GRAPH_INTERSECTION) != CALC_OK ||
        strstr(graph.analysis_text, "XING F1/F2") == NULL) {
        printf("FAIL: graph intersection analysis: %s\n", graph.analysis_text);
        failures++;
    }
    if (!calc_engine_uses_degrees()) {
        printf("FAIL: analysis did not restore DEG mode\n");
        failures++;
    }

    graph_model_set_function(&graph, 1, "");
    graph_model_set_range(&graph, 0.0, 3.14159265358979323846, -1.5, 1.5);
    graph.trace_x = 0.0;
    calculator_graph_analyze(&graph, 0.0, &symbols, CALCULATOR_GRAPH_ROOT);
    graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_ANALYSIS);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_ANALYSIS_MORE);

    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_MENU);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_ANALYSIS);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_TABLE);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_RANGE);

    calculator_widget_set_data_focus(true);
    if (calculator_widget_key_top(4) != 280) {
        printf("FAIL: data-focus graph keypad geometry\n");
        failures++;
    }
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_PLOT);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_TABLE);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_ANALYSIS);

    calculator_widget_set_layout(CALCULATOR_LAYOUT_FULLSCREEN);
    if (calculator_widget_key_top(4) != LCD_HEIGHT ||
        calculator_widget_key_height() != 0) {
        printf("FAIL: fullscreen graph keypad geometry\n");
        failures++;
    }
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_PLOT);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_TABLE);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_ANALYSIS);
    calculator_widget_set_layout(CALCULATOR_LAYOUT_STANDARD);

    lcd_set_orientation(LCD_ORIENTATION_PORTRAIT);
    if (lcd_width() != 320 || lcd_height() != 480 ||
        calculator_widget_key_top(4) != 404) {
        printf("FAIL: portrait graph geometry\n");
        failures++;
    }
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_PLOT);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_TABLE);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_ANALYSIS);
    calculator_widget_set_layout(CALCULATOR_LAYOUT_DATA_FOCUS);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_PLOT);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_TABLE);
    calculator_widget_set_layout(CALCULATOR_LAYOUT_FULLSCREEN);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_PLOT);
    expect_render_in_bounds(&graph, &symbols, GRAPH_VIEW_TABLE);
    calculator_widget_set_layout(CALCULATOR_LAYOUT_STANDARD);
    lcd_set_orientation(LCD_ORIENTATION_LANDSCAPE);

    if (failures) {
        printf("%d graph renderer test(s) failed\n", failures);
        return 1;
    }
    printf("graph renderer tests passed\n");
    return 0;
}
