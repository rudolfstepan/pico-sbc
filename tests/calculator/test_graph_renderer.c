#include "calculator_engine.h"
#include "calculator_graph.h"
#include "graph_model.h"
#include "lcd_st7796.h"
#include "mock_lcd.h"

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

static void expect_render_in_bounds(graph_model_t *graph, graph_view_t view) {
    char message[24] = "READY";
    graph_model_set_view(graph, view);
    calculator_widget_state_t state = widget_state(graph);
    mock_lcd_reset();
    calculator_graph_render(graph, 0.0, &state, message, sizeof message);
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
    calculator_graph_init(&graph);
    graph_model_set_function(&graph, 0, "sin(x)");

    calc_engine_set_degrees(true);
    if (calculator_graph_auto_scale(&graph, 0.0) != CALC_OK) {
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

    char message[24] = "READY";
    calculator_widget_state_t state = widget_state(&graph);
    mock_lcd_reset();
    calculator_graph_render(&graph, 0.0, &state, message, sizeof message);
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

    expect_render_in_bounds(&graph, GRAPH_VIEW_MENU);
    expect_render_in_bounds(&graph, GRAPH_VIEW_TABLE);
    expect_render_in_bounds(&graph, GRAPH_VIEW_RANGE);

    if (failures) {
        printf("%d graph renderer test(s) failed\n", failures);
        return 1;
    }
    printf("graph renderer tests passed\n");
    return 0;
}
