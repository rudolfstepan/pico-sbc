#ifndef CALCULATOR_CIRCUIT_H
#define CALCULATOR_CIRCUIT_H

#include "circuit_model.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CALCULATOR_CIRCUIT_TOOLBAR_HEIGHT 36
#define CALCULATOR_CIRCUIT_STATUS_HEIGHT 18
#define CALCULATOR_CIRCUIT_NODE_WIDTH 88
#define CALCULATOR_CIRCUIT_NODE_HEIGHT 54
#define CALCULATOR_CIRCUIT_ZOOM_LEVEL_COUNT 3u

enum {
    CALCULATOR_CIRCUIT_RENDER = 1u << 0,
    CALCULATOR_CIRCUIT_EXIT = 1u << 1,
    CALCULATOR_CIRCUIT_BEEP = 1u << 2,
    CALCULATOR_CIRCUIT_CHANGED = 1u << 3
};

typedef struct {
    circuit_model_t model;
    int viewport_x;
    int viewport_y;
    uint8_t zoom_index;
    uint8_t selected_node;
    uint8_t wire_source;
    circuit_gate_type_t add_type;
    bool add_armed;
    bool touch_active;
    bool touch_moved;
    uint8_t touch_target;
    uint8_t touch_node;
    uint8_t touch_input;
    uint8_t pressed_toolbar;
    uint16_t touch_start_x;
    uint16_t touch_start_y;
    uint16_t touch_last_x;
    uint16_t touch_last_y;
    int16_t touch_node_start_x;
    int16_t touch_node_start_y;
    char status[48];
} calculator_circuit_t;

void calculator_circuit_init(calculator_circuit_t *circuit);
void calculator_circuit_render(const calculator_circuit_t *circuit);
unsigned int calculator_circuit_touch(calculator_circuit_t *circuit,
                                      bool touched, uint16_t x, uint16_t y);
bool calculator_circuit_pan(calculator_circuit_t *circuit,
                            int delta_x, int delta_y,
                            int display_width, int display_height);
bool calculator_circuit_zoom(calculator_circuit_t *circuit, int direction,
                             int display_width, int display_height);
unsigned int calculator_circuit_zoom_percent(
    const calculator_circuit_t *circuit);
unsigned int calculator_circuit_activate_selected(
    calculator_circuit_t *circuit);

#endif
