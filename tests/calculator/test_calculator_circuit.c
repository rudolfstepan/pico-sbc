#include "calculator_circuit.h"
#include "lcd_st7796.h"
#include "mock_lcd.h"

#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static unsigned int tap(calculator_circuit_t *circuit, int x, int y) {
    (void)calculator_circuit_touch(circuit, true,
                                   (uint16_t)x, (uint16_t)y);
    /* A GT911 release frame has no valid coordinates. */
    return calculator_circuit_touch(circuit, false, 0u, 0u);
}

static size_t node_count(const calculator_circuit_t *circuit) {
    size_t count = 0;
    for (size_t i = 0; i < CIRCUIT_NODE_CAPACITY; ++i) {
        if (circuit->model.nodes[i].used) ++count;
    }
    return count;
}

static int test_orientation(lcd_orientation_t orientation) {
    calculator_circuit_t circuit;
    lcd_set_orientation(orientation);
    calculator_circuit_init(&circuit);
    for (uint8_t zoom = 0u;
         zoom < CALCULATOR_CIRCUIT_ZOOM_LEVEL_COUNT; ++zoom) {
        circuit.zoom_index = zoom;
        mock_lcd_reset();
        calculator_circuit_render(&circuit);
        if (mock_lcd_had_out_of_bounds_draw()) {
            fprintf(stderr, "orientation=%d zoom=%u\n",
                    (int)orientation, (unsigned int)zoom);
        }
        CHECK(!mock_lcd_had_out_of_bounds_draw());
        CHECK(mock_lcd_drew_text("HOME"));
        CHECK(mock_lcd_drew_text("LOGIC"));
        CHECK(mock_lcd_drew_text("Z-"));
        CHECK(mock_lcd_drew_text("Z+"));
        CHECK(mock_lcd_drew_text("+" LCD_TEXT_LOGIC_AND));
    }
    return 0;
}

int main(void) {
    CHECK(test_orientation(LCD_ORIENTATION_LANDSCAPE) == 0);
    CHECK(test_orientation(LCD_ORIENTATION_PORTRAIT) == 0);

    lcd_set_orientation(LCD_ORIENTATION_LANDSCAPE);
    calculator_circuit_t toolbar;
    calculator_circuit_init(&toolbar);
    unsigned int effect = tap(&toolbar, 150, 18);
    CHECK(effect & CALCULATOR_CIRCUIT_BEEP);
    CHECK(toolbar.add_type == CIRCUIT_GATE_OR);
    effect = tap(&toolbar, 100, 18);
    CHECK(toolbar.add_armed);
    effect = tap(&toolbar, 100, 18);
    CHECK(!toolbar.add_armed);
    toolbar.selected_node = 0u;
    effect = tap(&toolbar, 210, 18);
    CHECK(toolbar.wire_source == 0u);
    effect = tap(&toolbar, 210, 18);
    CHECK(toolbar.wire_source == CIRCUIT_NODE_NONE);
    CHECK(calculator_circuit_zoom_percent(&toolbar) == 150u);
    effect = tap(&toolbar, 390, 18);
    CHECK(effect & CALCULATOR_CIRCUIT_CHANGED);
    CHECK(calculator_circuit_zoom_percent(&toolbar) == 200u);
    effect = tap(&toolbar, 330, 18);
    CHECK(calculator_circuit_zoom_percent(&toolbar) == 150u);
    toolbar.selected_node = 2u;
    size_t toolbar_nodes = node_count(&toolbar);
    effect = tap(&toolbar, 270, 18);
    CHECK(effect & CALCULATOR_CIRCUIT_CHANGED);
    CHECK(node_count(&toolbar) == toolbar_nodes - 1u);
    effect = tap(&toolbar, 40, 18);
    CHECK(effect & CALCULATOR_CIRCUIT_EXIT);
    effect = tap(&toolbar, 450, 18);
    CHECK(effect & CALCULATOR_CIRCUIT_TO_LOGIC);

    calculator_circuit_t tolerant_touch;
    calculator_circuit_init(&tolerant_touch);
    effect = tap(&tolerant_touch, 182, 130);
    CHECK(tolerant_touch.wire_source == 0u);
    effect = tap(&tolerant_touch, 271, 213);
    CHECK(effect & CALCULATOR_CIRCUIT_CHANGED);

    calculator_circuit_t circuit;
    calculator_circuit_init(&circuit);

    effect = tap(&circuit, 60, 110);
    CHECK(effect & CALCULATOR_CIRCUIT_CHANGED);
    CHECK(circuit.model.nodes[0].input_value);

    (void)calculator_circuit_touch(&circuit, true, 320, 210);
    effect = calculator_circuit_touch(&circuit, true, 350, 240);
    CHECK(effect & CALCULATOR_CIRCUIT_CHANGED);
    effect = calculator_circuit_touch(&circuit, false, 0u, 0u);
    CHECK(effect & CALCULATOR_CIRCUIT_CHANGED);
    CHECK(circuit.model.nodes[2].x == 210);
    CHECK(circuit.model.nodes[2].y == 156);

    effect = tap(&circuit, 168, 130);
    CHECK(circuit.wire_source == 0u);
    effect = tap(&circuit, 315, 243);
    CHECK(effect & CALCULATOR_CIRCUIT_CHANGED);
    int wire = circuit_model_wire_for_input(&circuit.model, 2u, 0u);
    CHECK(wire >= 0);
    CHECK(circuit.model.wires[wire].source == 0u);

    circuit.selected_node = CIRCUIT_NODE_NONE;
    effect = tap(&circuit, 100, 18);
    CHECK(effect & CALCULATOR_CIRCUIT_BEEP);
    CHECK(circuit.add_armed);
    size_t before = node_count(&circuit);
    effect = tap(&circuit, 200, 360);
    CHECK(effect & CALCULATOR_CIRCUIT_CHANGED);
    CHECK(node_count(&circuit) == before + 1u);

    CHECK(calculator_circuit_pan(&circuit, 64, 64,
                                 lcd_width(), lcd_height()));
    CHECK(circuit.viewport_x == 42);
    CHECK(circuit.viewport_y == 42);
    int center_world_before = circuit.viewport_x +
        lcd_width() * 100 /
            (2 * (int)calculator_circuit_zoom_percent(&circuit));
    CHECK(calculator_circuit_zoom(&circuit, 1,
                                  lcd_width(), lcd_height()));
    CHECK(calculator_circuit_zoom_percent(&circuit) == 200u);
    int center_world_after = circuit.viewport_x +
        lcd_width() * 100 /
            (2 * (int)calculator_circuit_zoom_percent(&circuit));
    CHECK(center_world_after >= center_world_before - 1 &&
          center_world_after <= center_world_before + 1);
    calculator_circuit_render(&circuit);
    CHECK(!mock_lcd_had_out_of_bounds_draw());

    puts("calculator circuit tests passed");
    return 0;
}
