#include "calculator_circuit.h"

#include "calculator_ui_theme.h"
#include "lcd_st7796.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CIRCUIT_TOOL_COUNT 7u
#define CIRCUIT_PORT_TOUCH_RADIUS 12
#define CIRCUIT_DRAG_THRESHOLD 5

static const unsigned int zoom_levels[CALCULATOR_CIRCUIT_ZOOM_LEVEL_COUNT] = {
    100u, 150u, 200u
};

enum {
    TOUCH_TARGET_NONE,
    TOUCH_TARGET_TOOLBAR,
    TOUCH_TARGET_CANVAS,
    TOUCH_TARGET_NODE,
    TOUCH_TARGET_INPUT,
    TOUCH_TARGET_OUTPUT
};

static bool node_valid(const calculator_circuit_t *circuit, uint8_t node) {
    return circuit && node < CIRCUIT_NODE_CAPACITY &&
           circuit->model.nodes[node].used;
}

static int clamp_int(int value, int minimum, int maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static void set_status(calculator_circuit_t *circuit, const char *status) {
    snprintf(circuit->status, sizeof circuit->status, "%s", status);
}

unsigned int calculator_circuit_zoom_percent(
    const calculator_circuit_t *circuit) {
    if (!circuit || circuit->zoom_index >=
            CALCULATOR_CIRCUIT_ZOOM_LEVEL_COUNT) {
        return zoom_levels[0];
    }
    return zoom_levels[circuit->zoom_index];
}

static int scale_value(const calculator_circuit_t *circuit, int value) {
    return value * (int)calculator_circuit_zoom_percent(circuit) / 100;
}

static int unscale_value(const calculator_circuit_t *circuit, int value) {
    return value * 100 / (int)calculator_circuit_zoom_percent(circuit);
}

static int screen_x(const calculator_circuit_t *circuit, int world_x) {
    return scale_value(circuit, world_x - circuit->viewport_x);
}

static int screen_y(const calculator_circuit_t *circuit, int world_y) {
    return CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT + scale_value(
        circuit, world_y - circuit->viewport_y -
            CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT);
}

static int world_x(const calculator_circuit_t *circuit, int display_x) {
    return circuit->viewport_x + unscale_value(circuit, display_x);
}

static int world_y(const calculator_circuit_t *circuit, int display_y) {
    return circuit->viewport_y + CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT +
           unscale_value(circuit,
                         display_y - CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT);
}

static int input_port_y(const circuit_node_t *node, uint8_t input) {
    size_t count = circuit_gate_input_count(node->type);
    if (count < 2u) return node->y + CALCULATOR_CIRCUIT_NODE_HEIGHT / 2;
    return node->y + (input == 0u
        ? CALCULATOR_CIRCUIT_NODE_HEIGHT / 3
        : CALCULATOR_CIRCUIT_NODE_HEIGHT * 2 / 3);
}

static int output_port_y(const circuit_node_t *node) {
    return node->y + CALCULATOR_CIRCUIT_NODE_HEIGHT / 2;
}

static void clipped_fill_rect(int x, int y, int width, int height,
                              uint16_t color) {
    int left = x < 0 ? 0 : x;
    int top = y < CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT
        ? CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT : y;
    int right = x + width > lcd_width() ? lcd_width() : x + width;
    int canvas_bottom = lcd_height() - CALCULATOR_CIRCUIT_STATUS_HEIGHT;
    int bottom = y + height > canvas_bottom ? canvas_bottom : y + height;
    if (right > left && bottom > top) {
        lcd_fill_rect(left, top, right - left, bottom - top, color);
    }
}

static void draw_line(int x0, int y0, int x1, int y1,
                      uint16_t color, int thickness) {
    if (y0 == y1) {
        int left = x0 < x1 ? x0 : x1;
        int width = abs(x1 - x0) + thickness;
        clipped_fill_rect(left, y0 - thickness / 2, width, thickness, color);
        return;
    }
    if (x0 == x1) {
        int top = y0 < y1 ? y0 : y1;
        int height = abs(y1 - y0) + thickness;
        clipped_fill_rect(x0 - thickness / 2, top, thickness, height, color);
        return;
    }

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        clipped_fill_rect(x0 - thickness / 2, y0 - thickness / 2,
                          thickness, thickness, color);
        if (x0 == x1 && y0 == y1) break;
        int twice_error = error * 2;
        if (twice_error >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice_error <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

static void draw_circle_outline(int center_x, int center_y, int radius,
                                uint16_t color, int thickness) {
    static const int8_t profile[][2] = {
        {0, -4}, {3, -3}, {4, 0}, {3, 3},
        {0, 4}, {-3, 3}, {-4, 0}, {-3, -3}
    };
    int previous_x = center_x + profile[0][0] * radius / 4;
    int previous_y = center_y + profile[0][1] * radius / 4;
    for (size_t i = 1; i <= 8u; ++i) {
        size_t point = i % 8u;
        int next_x = center_x + profile[point][0] * radius / 4;
        int next_y = center_y + profile[point][1] * radius / 4;
        draw_line(previous_x, previous_y, next_x, next_y, color, thickness);
        previous_x = next_x;
        previous_y = next_y;
    }
}

static void draw_port(const calculator_circuit_t *circuit, int x, int y,
                      bool active, bool highlighted) {
    uint16_t color = active ? UI_COLOR_PRIMARY : UI_COLOR_MUTED;
    if (highlighted) color = UI_COLOR_DANGER;
    int outer = scale_value(circuit, 4);
    int inner = scale_value(circuit, 2);
    if (outer < 4) outer = 4;
    if (inner < 2) inner = 2;
    clipped_fill_rect(x - outer, y - outer,
                      outer * 2 + 1, outer * 2 + 1, color);
    clipped_fill_rect(x - inner, y - inner,
                      inner * 2 + 1, inner * 2 + 1,
                      UI_COLOR_BACKGROUND);
    clipped_fill_rect(x, y, 2, 2, color);
}

static void safe_text(int x, int y, const char *value, uint16_t foreground,
                      uint16_t background, uint8_t scale) {
    if (!value || x < 0 || x >= lcd_width() ||
        y < CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT ||
        y + 8 * scale >
            lcd_height() - CALCULATOR_CIRCUIT_STATUS_HEIGHT) {
        return;
    }
    size_t available = (size_t)(lcd_width() - x) / (6u * scale);
    if (available == 0u) return;
    char clipped[16];
    size_t length = strlen(value);
    if (length >= sizeof clipped) length = sizeof clipped - 1u;
    if (length > available) length = available;
    memcpy(clipped, value, length);
    clipped[length] = '\0';
    lcd_draw_text(x, y, clipped, foreground, background, scale);
}

static void draw_gate_shape(const calculator_circuit_t *circuit,
                            const circuit_node_t *node, int x, int y,
                            uint16_t color) {
    int node_width = scale_value(circuit, CALCULATOR_CIRCUIT_NODE_WIDTH);
    int node_height = scale_value(circuit, CALCULATOR_CIRCUIT_NODE_HEIGHT);
    int center_y = y + node_height / 2;
    int left = x + scale_value(circuit, 13);
    int right = x + scale_value(circuit, 73);
    int top = y + scale_value(circuit, 10);
    int bottom = y + node_height - scale_value(circuit, 10);
    int thickness = scale_value(circuit, 2);
    if (thickness < 2) thickness = 2;
    bool inverted = node->type == CIRCUIT_GATE_NAND ||
                    node->type == CIRCUIT_GATE_NOR ||
                    node->type == CIRCUIT_GATE_XNOR;

    if (node->type == CIRCUIT_GATE_INPUT) {
        draw_line(x + scale_value(circuit, 10), center_y,
                  right - scale_value(circuit, 8), center_y,
                  color, thickness);
        draw_line(right - scale_value(circuit, 14),
                  center_y - scale_value(circuit, 7),
                  right - scale_value(circuit, 7), center_y,
                  color, thickness);
        draw_line(right - scale_value(circuit, 14),
                  center_y + scale_value(circuit, 7),
                  right - scale_value(circuit, 7), center_y,
                  color, thickness);
    } else if (node->type == CIRCUIT_GATE_OUTPUT) {
        draw_line(left, center_y, right - scale_value(circuit, 15),
                  center_y, color, thickness);
        draw_circle_outline(right - scale_value(circuit, 8), center_y,
                            scale_value(circuit, 10), color, thickness);
    } else if (node->type == CIRCUIT_GATE_NOT) {
        draw_line(left, top, left, bottom, color, thickness);
        draw_line(left, top, right - scale_value(circuit, 5), center_y,
                  color, thickness);
        draw_line(left, bottom, right - scale_value(circuit, 5), center_y,
                  color, thickness);
        draw_circle_outline(right, center_y, scale_value(circuit, 5),
                            color, thickness);
    } else if (node->type == CIRCUIT_GATE_AND ||
               node->type == CIRCUIT_GATE_NAND) {
        int x45 = x + scale_value(circuit, 45);
        int x62 = x + scale_value(circuit, 62);
        int y14 = y + scale_value(circuit, 14);
        draw_line(left, top, left, bottom, color, thickness);
        draw_line(left, top, x45, top, color, thickness);
        draw_line(left, bottom, x45, bottom, color, thickness);
        draw_line(x45, top, x62, y14, color, thickness);
        draw_line(x62, y14, right, center_y, color, thickness);
        draw_line(right, center_y, x62,
                  bottom - scale_value(circuit, 4), color, thickness);
        draw_line(x62, bottom - scale_value(circuit, 4), x45, bottom,
                  color, thickness);
    } else {
        int extra = (node->type == CIRCUIT_GATE_XOR ||
                     node->type == CIRCUIT_GATE_XNOR)
            ? scale_value(circuit, 5) : 0;
        int x43 = x + scale_value(circuit, 43);
        int y13 = y + scale_value(circuit, 13);
        draw_line(left + extra, top, x43, y13, color, thickness);
        draw_line(x43, y13, right, center_y, color, thickness);
        draw_line(right, center_y, x43,
                  bottom - scale_value(circuit, 3), color, thickness);
        draw_line(x43, bottom - scale_value(circuit, 3), left + extra,
                  bottom, color, thickness);
        draw_line(left + extra, top,
                  x + scale_value(circuit, 25) + extra, center_y,
                  color, thickness);
        draw_line(x + scale_value(circuit, 25) + extra, center_y,
                  left + extra, bottom, color, thickness);
        if (extra) {
            draw_line(left, top, x + scale_value(circuit, 20), center_y,
                      color, thickness);
            draw_line(x + scale_value(circuit, 20), center_y, left, bottom,
                      color, thickness);
        }
    }

    size_t inputs = circuit_gate_input_count(node->type);
    for (uint8_t input = 0; input < inputs; ++input) {
        int port_y = y + scale_value(
            circuit, input_port_y(node, input) - node->y);
        draw_line(x, port_y, left, port_y, color, thickness);
    }
    if (circuit_gate_has_output(node->type)) {
        int shape_end = right;
        if (node->type == CIRCUIT_GATE_INPUT) {
            shape_end = right - scale_value(circuit, 7);
        }
        if (node->type == CIRCUIT_GATE_NOT) {
            shape_end = right + scale_value(circuit, 5);
        }
        if (inverted) {
            draw_circle_outline(right + scale_value(circuit, 4), center_y,
                                scale_value(circuit, 5), color, thickness);
            shape_end = right + scale_value(circuit, 9);
        }
        draw_line(shape_end, center_y, x + node_width, center_y,
                  color, thickness);
    }
}

static void draw_node(const calculator_circuit_t *circuit, uint8_t index) {
    const circuit_node_t *node = &circuit->model.nodes[index];
    int x = screen_x(circuit, node->x);
    int y = screen_y(circuit, node->y);
    int node_width = scale_value(circuit, CALCULATOR_CIRCUIT_NODE_WIDTH);
    int node_height = scale_value(circuit, CALCULATOR_CIRCUIT_NODE_HEIGHT);
    uint8_t text_scale = calculator_circuit_zoom_percent(circuit) >= 150u
        ? 2u : 1u;
    if (x >= lcd_width() || x + node_width < 0 ||
        y >= lcd_height() - CALCULATOR_CIRCUIT_STATUS_HEIGHT ||
        y + node_height < CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT) {
        return;
    }

    uint16_t color = index == circuit->selected_node
        ? UI_COLOR_PRIMARY : UI_COLOR_TEXT;
    draw_gate_shape(circuit, node, x, y, color);
    safe_text(x + scale_value(circuit, 4), y + scale_value(circuit, 1),
              node->label, color, UI_COLOR_BACKGROUND, text_scale);
    if (node->type != CIRCUIT_GATE_INPUT &&
        node->type != CIRCUIT_GATE_OUTPUT) {
        const char *name = circuit_gate_name(node->type);
        int text_width = (int)strlen(name) * 6 * text_scale;
        int text_x = x + (node_width - text_width) / 2;
        int text_y = y + (node_height - 8 * text_scale) / 2;
        safe_text(text_x, text_y, name, UI_COLOR_MUTED,
                  UI_COLOR_BACKGROUND, text_scale);
    }
    char value[2] = {node->output_value ? '1' : '0', '\0'};
    safe_text(x + node_width - 6 * text_scale - scale_value(circuit, 4),
              y + scale_value(circuit, 1), value,
              node->output_value ? UI_COLOR_PRIMARY : UI_COLOR_MUTED,
              UI_COLOR_BACKGROUND, text_scale);

    size_t inputs = circuit_gate_input_count(node->type);
    for (uint8_t input = 0; input < inputs; ++input) {
        int wire = circuit_model_wire_for_input(&circuit->model, index, input);
        draw_port(circuit, x,
                  screen_y(circuit, input_port_y(node, input)),
                  wire >= 0, false);
    }
    if (circuit_gate_has_output(node->type)) {
        draw_port(circuit, x + node_width,
                  screen_y(circuit, output_port_y(node)),
                  node->output_value, index == circuit->wire_source);
    }

    if (index == circuit->selected_node) {
        draw_line(x + 1, y + 1,
                  x + node_width - 1, y + 1,
                  UI_COLOR_PRIMARY, 1);
        draw_line(x + 1, y + node_height - 1,
                  x + node_width - 1,
                  y + node_height - 1,
                  UI_COLOR_PRIMARY, 1);
    }
}

static void draw_wire(const calculator_circuit_t *circuit,
                      const circuit_wire_t *wire) {
    if (!wire->used || !node_valid(circuit, wire->source) ||
        !node_valid(circuit, wire->destination)) {
        return;
    }
    const circuit_node_t *source = &circuit->model.nodes[wire->source];
    const circuit_node_t *destination =
        &circuit->model.nodes[wire->destination];
    int x0 = screen_x(circuit,
                      source->x + CALCULATOR_CIRCUIT_NODE_WIDTH);
    int y0 = screen_y(circuit, output_port_y(source));
    int x1 = screen_x(circuit, destination->x);
    int y1 = screen_y(circuit,
                      input_port_y(destination, wire->destination_input));
    int middle = x0 + (x1 - x0) / 2;
    uint16_t color = source->output_value
        ? UI_COLOR_PRIMARY : UI_COLOR_GRID;
    int thickness = scale_value(circuit, 2);
    if (thickness < 2) thickness = 2;
    draw_line(x0, y0, middle, y0, color, thickness);
    draw_line(middle, y0, middle, y1, color, thickness);
    draw_line(middle, y1, x1, y1, color, thickness);
}

static void draw_grid(const calculator_circuit_t *circuit) {
    const int spacing = 40;
    int grid_x = circuit->viewport_x - circuit->viewport_x % spacing;
    while (screen_x(circuit, grid_x) < 0) grid_x += spacing;
    for (int x = screen_x(circuit, grid_x); x < lcd_width();
         grid_x += spacing, x = screen_x(circuit, grid_x)) {
        clipped_fill_rect(x, CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT, 1,
                          lcd_height(), UI_COLOR_GRID);
    }
    int visible_top = circuit->viewport_y +
                      CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT;
    int grid_y = visible_top - visible_top % spacing;
    while (screen_y(circuit, grid_y) <
           CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT) {
        grid_y += spacing;
    }
    for (int y = screen_y(circuit, grid_y);
         y < lcd_height() - CALCULATOR_CIRCUIT_STATUS_HEIGHT;
         grid_y += spacing, y = screen_y(circuit, grid_y)) {
        clipped_fill_rect(0, y, lcd_width(), 1, UI_COLOR_GRID);
    }
}

static void draw_toolbar(const calculator_circuit_t *circuit) {
    char add_label[10];
    snprintf(add_label, sizeof add_label, "+%s",
             circuit_gate_name(circuit->add_type));
    const char *labels[CIRCUIT_TOOL_COUNT] = {
        "HOME", add_label, "TYPE", "LINK", "DEL", "Z-", "Z+"
    };
    int width = lcd_width();
    lcd_fill_rect(0, 0, width, CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT,
                  UI_COLOR_SURFACE);
    for (unsigned int tool = 0; tool < CIRCUIT_TOOL_COUNT; ++tool) {
        int left = (int)(tool * (unsigned int)width / CIRCUIT_TOOL_COUNT);
        int right = (int)((tool + 1u) * (unsigned int)width /
                          CIRCUIT_TOOL_COUNT);
        bool active = circuit->pressed_toolbar == tool ||
            (tool == 1u && circuit->add_armed) ||
            (tool == 3u && circuit->wire_source != CIRCUIT_NODE_NONE);
        uint16_t background = active ? UI_COLOR_PRIMARY : UI_COLOR_SURFACE;
        uint16_t foreground = active ? UI_COLOR_TEXT_DARK : UI_COLOR_TEXT;
        lcd_fill_rect(left + 1, 1, right - left - 2,
                      CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT - 3, background);
        lcd_draw_rect(left, 0, right - left,
                      CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT, UI_COLOR_MUTED);
        size_t length = strlen(labels[tool]);
        uint8_t scale = length * 12u <= (size_t)(right - left - 4) ? 2u : 1u;
        int text_width = (int)(length * 6u * scale);
        int text_x = left + (right - left - text_width) / 2;
        int text_y = (CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT - 8 * scale) / 2;
        lcd_draw_text(text_x, text_y, labels[tool], foreground,
                      background, scale);
    }
}

static void draw_status(const calculator_circuit_t *circuit) {
    int top = lcd_height() - CALCULATOR_CIRCUIT_STATUS_HEIGHT;
    lcd_fill_rect(0, top, lcd_width(), CALCULATOR_CIRCUIT_STATUS_HEIGHT,
                  UI_COLOR_BACKGROUND);
    lcd_fill_rect(0, top, lcd_width(), 1, UI_COLOR_MUTED);
    char status[80];
    snprintf(status, sizeof status, "%s Z%u%% X%d Y%d",
             circuit->status, calculator_circuit_zoom_percent(circuit),
             circuit->viewport_x, circuit->viewport_y);
    size_t capacity = (size_t)(lcd_width() - 8) / 6u;
    if (capacity >= sizeof status) capacity = sizeof status - 1u;
    status[capacity] = '\0';
    lcd_draw_text(4, top + 5, status, UI_COLOR_TEXT,
                  UI_COLOR_BACKGROUND, 1);
}

void calculator_circuit_render(const calculator_circuit_t *circuit) {
    if (!circuit) return;
    lcd_fill(UI_COLOR_BACKGROUND);
    draw_grid(circuit);
    for (size_t wire = 0; wire < CIRCUIT_WIRE_CAPACITY; ++wire) {
        draw_wire(circuit, &circuit->model.wires[wire]);
    }
    for (uint8_t node = 0; node < CIRCUIT_NODE_CAPACITY; ++node) {
        if (circuit->model.nodes[node].used) draw_node(circuit, node);
    }
    draw_toolbar(circuit);
    draw_status(circuit);
}

void calculator_circuit_init(calculator_circuit_t *circuit) {
    if (!circuit) return;
    memset(circuit, 0, sizeof *circuit);
    circuit_model_init(&circuit->model);
    circuit->zoom_index = 1u;
    circuit->selected_node = CIRCUIT_NODE_NONE;
    circuit->wire_source = CIRCUIT_NODE_NONE;
    circuit->add_type = CIRCUIT_GATE_AND;
    circuit->pressed_toolbar = CIRCUIT_NODE_NONE;
    set_status(circuit, "TAP INPUT OR DRAG GATE");
}

static int point_distance_squared(int x, int y, int target_x, int target_y) {
    int delta_x = x - target_x;
    int delta_y = y - target_y;
    return delta_x * delta_x + delta_y * delta_y;
}

static int port_touch_radius(const calculator_circuit_t *circuit) {
    int radius = scale_value(circuit, CIRCUIT_PORT_TOUCH_RADIUS);
    return radius < CIRCUIT_PORT_TOUCH_RADIUS
        ? CIRCUIT_PORT_TOUCH_RADIUS : radius;
}

static void identify_touch(calculator_circuit_t *circuit, int x, int y) {
    circuit->touch_target = TOUCH_TARGET_CANVAS;
    circuit->touch_node = CIRCUIT_NODE_NONE;
    circuit->touch_input = 0u;
    if (y < CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT) {
        circuit->touch_target = TOUCH_TARGET_TOOLBAR;
        int tool = x * (int)CIRCUIT_TOOL_COUNT / lcd_width();
        circuit->pressed_toolbar = (uint8_t)clamp_int(
            tool, 0, (int)CIRCUIT_TOOL_COUNT - 1);
        return;
    }

    for (int index = (int)CIRCUIT_NODE_CAPACITY - 1; index >= 0; --index) {
        const circuit_node_t *node = &circuit->model.nodes[index];
        if (!node->used) continue;
        int node_x = screen_x(circuit, node->x);
        int radius = port_touch_radius(circuit);
        int maximum_distance = radius * radius;
        size_t inputs = circuit_gate_input_count(node->type);
        int closest_input = -1;
        int closest_distance = maximum_distance + 1;
        for (uint8_t input = 0; input < inputs; ++input) {
            int port_y = screen_y(circuit, input_port_y(node, input));
            int distance = point_distance_squared(x, y, node_x, port_y);
            if (distance <= maximum_distance &&
                distance < closest_distance) {
                closest_input = input;
                closest_distance = distance;
            }
        }
        if (closest_input >= 0) {
            circuit->touch_target = TOUCH_TARGET_INPUT;
            circuit->touch_node = (uint8_t)index;
            circuit->touch_input = (uint8_t)closest_input;
            return;
        }
        int node_width = scale_value(circuit,
                                     CALCULATOR_CIRCUIT_NODE_WIDTH);
        if (circuit_gate_has_output(node->type) &&
            point_distance_squared(
                x, y, node_x + node_width,
                screen_y(circuit, output_port_y(node))) <=
                    maximum_distance) {
            circuit->touch_target = TOUCH_TARGET_OUTPUT;
            circuit->touch_node = (uint8_t)index;
            return;
        }
        int node_y = screen_y(circuit, node->y);
        int node_height = scale_value(circuit,
                                      CALCULATOR_CIRCUIT_NODE_HEIGHT);
        if (x >= node_x && x <= node_x + node_width &&
            y >= node_y && y <= node_y + node_height) {
            circuit->touch_target = TOUCH_TARGET_NODE;
            circuit->touch_node = (uint8_t)index;
            return;
        }
    }
}

static bool gate_position_is_free(const calculator_circuit_t *circuit,
                                  int x, int y) {
    const int margin = 12;
    for (size_t i = 0; i < CIRCUIT_NODE_CAPACITY; ++i) {
        const circuit_node_t *node = &circuit->model.nodes[i];
        if (!node->used) continue;
        if (x < node->x + CALCULATOR_CIRCUIT_NODE_WIDTH + margin &&
            x + CALCULATOR_CIRCUIT_NODE_WIDTH + margin > node->x &&
            y < node->y + CALCULATOR_CIRCUIT_NODE_HEIGHT + margin &&
            y + CALCULATOR_CIRCUIT_NODE_HEIGHT + margin > node->y) {
            return false;
        }
    }
    return true;
}

static void find_free_gate_position(const calculator_circuit_t *circuit,
                                    int *x, int *y) {
    static const int offsets[][2] = {
        {0, 0}, {0, 78}, {0, -78}, {100, 0}, {-100, 0},
        {100, 78}, {100, -78}, {-100, 78}, {-100, -78},
        {0, 156}, {0, -156}
    };
    int original_x = *x;
    int original_y = *y;
    for (size_t i = 0; i < sizeof offsets / sizeof offsets[0]; ++i) {
        int candidate_x = clamp_int(
            original_x + offsets[i][0], 0,
            CIRCUIT_WORLD_WIDTH - CALCULATOR_CIRCUIT_NODE_WIDTH);
        int candidate_y = clamp_int(
            original_y + offsets[i][1],
            CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT,
            CIRCUIT_WORLD_HEIGHT - CALCULATOR_CIRCUIT_NODE_HEIGHT);
        if (gate_position_is_free(circuit, candidate_x, candidate_y)) {
            *x = candidate_x;
            *y = candidate_y;
            return;
        }
    }
}

static bool add_gate(calculator_circuit_t *circuit, int world_x, int world_y,
                     uint8_t *added) {
    world_x = clamp_int(world_x, 0,
        CIRCUIT_WORLD_WIDTH - CALCULATOR_CIRCUIT_NODE_WIDTH);
    world_y = clamp_int(world_y, CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT,
        CIRCUIT_WORLD_HEIGHT - CALCULATOR_CIRCUIT_NODE_HEIGHT);
    find_free_gate_position(circuit, &world_x, &world_y);
    int node = circuit_model_add(&circuit->model, circuit->add_type,
                                 world_x, world_y);
    if (node < 0) {
        set_status(circuit, "NO FREE GATE SLOT");
        return false;
    }
    *added = (uint8_t)node;
    circuit->selected_node = (uint8_t)node;
    circuit->add_armed = false;
    char status[48];
    snprintf(status, sizeof status, "%s ADDED",
             circuit_gate_name(circuit->add_type));
    set_status(circuit, status);
    return true;
}

static unsigned int add_at_port(calculator_circuit_t *circuit,
                                uint8_t node, uint8_t input,
                                bool at_output) {
    const circuit_node_t *anchor = &circuit->model.nodes[node];
    uint8_t added = CIRCUIT_NODE_NONE;
    if (at_output) {
        if (circuit_gate_input_count(circuit->add_type) == 0u) {
            set_status(circuit, "TYPE NEEDS AN INPUT");
            return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP;
        }
        if (!add_gate(circuit,
                      anchor->x + CALCULATOR_CIRCUIT_NODE_WIDTH + 64,
                      anchor->y, &added)) {
            return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP;
        }
        (void)circuit_model_connect(&circuit->model, node, added, 0u);
    } else {
        if (!circuit_gate_has_output(circuit->add_type)) {
            set_status(circuit, "TYPE HAS NO OUTPUT");
            return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP;
        }
        if (!add_gate(circuit,
                      anchor->x - CALCULATOR_CIRCUIT_NODE_WIDTH - 64,
                      anchor->y, &added)) {
            return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP;
        }
        (void)circuit_model_connect(&circuit->model, added, node, input);
    }
    circuit_model_evaluate(&circuit->model);
    return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP |
           CALCULATOR_CIRCUIT_CHANGED;
}

static void clamp_viewport(calculator_circuit_t *circuit,
                           int display_width, int display_height) {
    int canvas_height = display_height - CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT -
                        CALCULATOR_CIRCUIT_STATUS_HEIGHT;
    int visible_width = unscale_value(circuit, display_width);
    int visible_height = unscale_value(circuit, canvas_height);
    int maximum_x = CIRCUIT_WORLD_WIDTH - visible_width;
    int maximum_y = CIRCUIT_WORLD_HEIGHT -
                    CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT - visible_height;
    if (maximum_x < 0) maximum_x = 0;
    if (maximum_y < 0) maximum_y = 0;
    circuit->viewport_x = clamp_int(circuit->viewport_x, 0, maximum_x);
    circuit->viewport_y = clamp_int(circuit->viewport_y, 0, maximum_y);
}

bool calculator_circuit_zoom(calculator_circuit_t *circuit, int direction,
                             int display_width, int display_height) {
    if (!circuit || display_width <= 0 || display_height <=
            CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT +
                CALCULATOR_CIRCUIT_STATUS_HEIGHT) {
        return false;
    }
    uint8_t old_index = circuit->zoom_index <
            CALCULATOR_CIRCUIT_ZOOM_LEVEL_COUNT
        ? circuit->zoom_index : 0u;
    uint8_t new_index;
    if (direction == 0) {
        new_index = (uint8_t)((old_index + 1u) %
                              CALCULATOR_CIRCUIT_ZOOM_LEVEL_COUNT);
    } else {
        int requested = (int)old_index + (direction < 0 ? -1 : 1);
        new_index = (uint8_t)clamp_int(
            requested, 0, (int)CALCULATOR_CIRCUIT_ZOOM_LEVEL_COUNT - 1);
    }
    if (new_index == old_index) {
        set_status(circuit, direction < 0 ? "ZOOM MIN" : "ZOOM MAX");
        return false;
    }

    int canvas_center_y = CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT +
        (display_height - CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT -
         CALCULATOR_CIRCUIT_STATUS_HEIGHT) / 2;
    int center_world_x = world_x(circuit, display_width / 2);
    int center_world_y = world_y(circuit, canvas_center_y);
    circuit->zoom_index = new_index;
    circuit->viewport_x = center_world_x -
        unscale_value(circuit, display_width / 2);
    circuit->viewport_y = center_world_y -
        CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT -
        unscale_value(circuit,
                      canvas_center_y - CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT);
    clamp_viewport(circuit, display_width, display_height);
    char status[48];
    snprintf(status, sizeof status, "ZOOM %u%%",
             calculator_circuit_zoom_percent(circuit));
    set_status(circuit, status);
    return true;
}

static unsigned int activate_toolbar(calculator_circuit_t *circuit,
                                     uint8_t tool) {
    switch (tool) {
        case 0u:
            return CALCULATOR_CIRCUIT_EXIT | CALCULATOR_CIRCUIT_BEEP;
        case 1u:
            circuit->add_armed = !circuit->add_armed;
            circuit->wire_source = CIRCUIT_NODE_NONE;
            set_status(circuit, circuit->add_armed
                ? "TAP SPACE OR A PORT" : "ADD CANCELLED");
            break;
        case 2u:
            if (node_valid(circuit, circuit->selected_node)) {
                circuit_gate_type_t type = (circuit_gate_type_t)(
                    (circuit->model.nodes[circuit->selected_node].type + 1u) %
                    CIRCUIT_GATE_COUNT);
                circuit_model_set_type(&circuit->model,
                                       circuit->selected_node, type);
                circuit->add_type = type;
                char status[48];
                snprintf(status, sizeof status, "GATE IS %s",
                         circuit_gate_name(type));
                set_status(circuit, status);
                return CALCULATOR_CIRCUIT_RENDER |
                       CALCULATOR_CIRCUIT_BEEP |
                       CALCULATOR_CIRCUIT_CHANGED;
            }
            circuit->add_type = (circuit_gate_type_t)(
                (circuit->add_type + 1u) % CIRCUIT_GATE_COUNT);
            set_status(circuit, "ADD TYPE CHANGED");
            break;
        case 3u:
            if (circuit->wire_source != CIRCUIT_NODE_NONE) {
                circuit->wire_source = CIRCUIT_NODE_NONE;
                set_status(circuit, "LINK CANCELLED");
            } else if (node_valid(circuit, circuit->selected_node) &&
                       circuit_gate_has_output(circuit->model.nodes[
                           circuit->selected_node].type)) {
                circuit->wire_source = circuit->selected_node;
                circuit->add_armed = false;
                set_status(circuit, "TAP DESTINATION INPUT");
            } else {
                set_status(circuit, "SELECT A SOURCE GATE");
            }
            break;
        case 4u:
            if (node_valid(circuit, circuit->selected_node)) {
                uint8_t removed = circuit->selected_node;
                circuit_model_remove(&circuit->model, removed);
                if (circuit->wire_source == removed) {
                    circuit->wire_source = CIRCUIT_NODE_NONE;
                }
                circuit->selected_node = CIRCUIT_NODE_NONE;
                set_status(circuit, "GATE DELETED");
                return CALCULATOR_CIRCUIT_RENDER |
                       CALCULATOR_CIRCUIT_BEEP |
                       CALCULATOR_CIRCUIT_CHANGED;
            }
            set_status(circuit, "SELECT GATE TO DELETE");
            break;
        case 5u:
        case 6u: {
            bool changed = calculator_circuit_zoom(
                circuit, tool == 5u ? -1 : 1, lcd_width(), lcd_height());
            return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP |
                   (changed ? CALCULATOR_CIRCUIT_CHANGED : 0u);
        }
        default:
            break;
    }
    return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP;
}

static unsigned int activate_port(calculator_circuit_t *circuit,
                                  bool output) {
    uint8_t node = circuit->touch_node;
    if (!node_valid(circuit, node)) return CALCULATOR_CIRCUIT_RENDER;
    circuit->selected_node = node;
    if (circuit->add_armed) {
        return add_at_port(circuit, node, circuit->touch_input, output);
    }
    if (output) {
        if (circuit->wire_source == node) {
            circuit->wire_source = CIRCUIT_NODE_NONE;
            set_status(circuit, "LINK CANCELLED");
        } else {
            circuit->wire_source = node;
            set_status(circuit, "TAP DESTINATION INPUT");
        }
        return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP;
    }

    if (circuit->wire_source != CIRCUIT_NODE_NONE) {
        bool connected = circuit_model_connect(
            &circuit->model, circuit->wire_source, node,
            circuit->touch_input);
        circuit->wire_source = CIRCUIT_NODE_NONE;
        set_status(circuit, connected ? "LINK CONNECTED" : "LINK REJECTED");
        return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP |
               (connected ? CALCULATOR_CIRCUIT_CHANGED : 0u);
    }
    bool disconnected = circuit_model_disconnect_input(
        &circuit->model, node, circuit->touch_input);
    set_status(circuit, disconnected ? "INPUT DISCONNECTED" :
                                      "TAP AN OUTPUT FIRST");
    return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP |
           (disconnected ? CALCULATOR_CIRCUIT_CHANGED : 0u);
}

unsigned int calculator_circuit_touch(calculator_circuit_t *circuit,
                                      bool touched, uint16_t x, uint16_t y) {
    if (!circuit) return 0u;
    if (touched && !circuit->touch_active) {
        circuit->touch_active = true;
        circuit->touch_moved = false;
        circuit->touch_start_x = x;
        circuit->touch_start_y = y;
        circuit->touch_last_x = x;
        circuit->touch_last_y = y;
        identify_touch(circuit, x, y);
        if (circuit->touch_target == TOUCH_TARGET_NODE) {
            circuit->selected_node = circuit->touch_node;
            circuit->touch_node_start_x =
                circuit->model.nodes[circuit->touch_node].x;
            circuit->touch_node_start_y =
                circuit->model.nodes[circuit->touch_node].y;
        }
        return CALCULATOR_CIRCUIT_RENDER;
    }
    if (touched && circuit->touch_active) {
        if (circuit->touch_target == TOUCH_TARGET_NODE &&
            !circuit->add_armed && node_valid(circuit, circuit->touch_node)) {
            int total_x = (int)x - (int)circuit->touch_start_x;
            int total_y = (int)y - (int)circuit->touch_start_y;
            if (circuit->touch_moved || abs(total_x) >= CIRCUIT_DRAG_THRESHOLD ||
                abs(total_y) >= CIRCUIT_DRAG_THRESHOLD) {
                circuit_model_move(
                    &circuit->model, circuit->touch_node,
                    clamp_int(circuit->touch_node_start_x +
                                  unscale_value(circuit, total_x), 0,
                              CIRCUIT_WORLD_WIDTH -
                                  CALCULATOR_CIRCUIT_NODE_WIDTH),
                    clamp_int(circuit->touch_node_start_y +
                                  unscale_value(circuit, total_y),
                              CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT,
                              CIRCUIT_WORLD_HEIGHT -
                                  CALCULATOR_CIRCUIT_NODE_HEIGHT));
                circuit->touch_moved = true;
                circuit->touch_last_x = x;
                circuit->touch_last_y = y;
                set_status(circuit, "MOVING GATE");
                return CALCULATOR_CIRCUIT_RENDER |
                       CALCULATOR_CIRCUIT_CHANGED;
            }
        }
        circuit->touch_last_x = x;
        circuit->touch_last_y = y;
        return 0u;
    }
    if (touched || !circuit->touch_active) return 0u;

    circuit->touch_active = false;
    uint8_t pressed_toolbar = circuit->pressed_toolbar;
    circuit->pressed_toolbar = CIRCUIT_NODE_NONE;
    if (circuit->touch_moved) {
        circuit->touch_moved = false;
        set_status(circuit, "GATE MOVED");
        return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP |
               CALCULATOR_CIRCUIT_CHANGED;
    }
    switch (circuit->touch_target) {
        case TOUCH_TARGET_TOOLBAR: {
            /* GT911 release reports contain no point coordinates. The tool
             * is checked against the last valid touched position instead. */
            uint8_t last_tool = (uint8_t)clamp_int(
                (int)circuit->touch_last_x * (int)CIRCUIT_TOOL_COUNT /
                    lcd_width(),
                0, (int)CIRCUIT_TOOL_COUNT - 1);
            if (circuit->touch_last_y >= CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT ||
                last_tool != pressed_toolbar) {
                return CALCULATOR_CIRCUIT_RENDER;
            }
            return activate_toolbar(circuit, pressed_toolbar);
        }
        case TOUCH_TARGET_INPUT:
            return activate_port(circuit, false);
        case TOUCH_TARGET_OUTPUT:
            return activate_port(circuit, true);
        case TOUCH_TARGET_NODE:
            if (node_valid(circuit, circuit->touch_node) &&
                circuit->model.nodes[circuit->touch_node].type ==
                    CIRCUIT_GATE_INPUT) {
                circuit_model_toggle_input(&circuit->model,
                                           circuit->touch_node);
                set_status(circuit,
                           circuit->model.nodes[circuit->touch_node].input_value
                               ? "INPUT IS 1" : "INPUT IS 0");
                return CALCULATOR_CIRCUIT_RENDER |
                       CALCULATOR_CIRCUIT_BEEP |
                       CALCULATOR_CIRCUIT_CHANGED;
            }
            set_status(circuit, "GATE SELECTED");
            return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP;
        case TOUCH_TARGET_CANVAS:
            if (circuit->add_armed) {
                uint8_t added = CIRCUIT_NODE_NONE;
                bool ok = add_gate(circuit,
                    world_x(circuit, circuit->touch_last_x) -
                        CALCULATOR_CIRCUIT_NODE_WIDTH / 2,
                    world_y(circuit, circuit->touch_last_y) -
                        CALCULATOR_CIRCUIT_NODE_HEIGHT / 2,
                    &added);
                return CALCULATOR_CIRCUIT_RENDER |
                       CALCULATOR_CIRCUIT_BEEP |
                       (ok ? CALCULATOR_CIRCUIT_CHANGED : 0u);
            }
            circuit->selected_node = CIRCUIT_NODE_NONE;
            circuit->wire_source = CIRCUIT_NODE_NONE;
            set_status(circuit, "CANVAS");
            return CALCULATOR_CIRCUIT_RENDER;
        default:
            return CALCULATOR_CIRCUIT_RENDER;
    }
}

bool calculator_circuit_pan(calculator_circuit_t *circuit,
                            int delta_x, int delta_y,
                            int display_width, int display_height) {
    if (!circuit) return false;
    int old_x = circuit->viewport_x;
    int old_y = circuit->viewport_y;
    circuit->viewport_x += unscale_value(circuit, delta_x);
    circuit->viewport_y += unscale_value(circuit, delta_y);
    clamp_viewport(circuit, display_width, display_height);
    int new_x = circuit->viewport_x;
    int new_y = circuit->viewport_y;
    if (new_x == old_x && new_y == old_y) {
        return false;
    }
    set_status(circuit, "JOYSTICK PAN");
    return true;
}

unsigned int calculator_circuit_activate_selected(
    calculator_circuit_t *circuit) {
    if (!node_valid(circuit, circuit->selected_node)) {
        set_status(circuit, "SELECT A GATE");
        return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP;
    }
    circuit_node_t *node = &circuit->model.nodes[circuit->selected_node];
    if (node->type == CIRCUIT_GATE_INPUT) {
        circuit_model_toggle_input(&circuit->model,
                                   circuit->selected_node);
        set_status(circuit, node->input_value ? "INPUT IS 1" : "INPUT IS 0");
        return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP |
               CALCULATOR_CIRCUIT_CHANGED;
    }
    if (circuit_gate_has_output(node->type)) {
        circuit->wire_source = circuit->selected_node;
        circuit->add_armed = false;
        set_status(circuit, "TAP DESTINATION INPUT");
    } else {
        set_status(circuit, "OUTPUT SELECTED");
    }
    return CALCULATOR_CIRCUIT_RENDER | CALCULATOR_CIRCUIT_BEEP;
}
