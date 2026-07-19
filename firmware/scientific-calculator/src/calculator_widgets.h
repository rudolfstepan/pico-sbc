#ifndef CALCULATOR_WIDGETS_H
#define CALCULATOR_WIDGETS_H

#include "calculator_ui_types.h"
#include "graph_model.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CALCULATOR_DISPLAY_HEIGHT 84
#define CALCULATOR_KEY_X 4
#define CALCULATOR_KEY_Y 88
#define CALCULATOR_KEY_WIDTH 75
#define CALCULATOR_KEY_HEIGHT 42
#define CALCULATOR_KEY_GAP_X 4
#define CALCULATOR_KEY_GAP_Y 4

typedef struct {
    calc_page_t page;
    unsigned int programmer_base;
    unsigned int format_bits;
    calculator_format_view_t format_view;
    bool programmer_signed;
    bool degrees;
    graph_view_t graph_view;
    unsigned int graph_active_mask;
    size_t graph_selected_function;
    const char *favorites[6];
} calculator_widget_state_t;

void calculator_widget_draw_key(const calc_key_t *key, bool pressed,
                                const calculator_widget_state_t *state);
void calculator_widget_render_keypad(calc_page_t page,
                                     const calculator_widget_state_t *state);
const calc_key_t *calculator_widget_hit_key(calc_page_t page,
                                            const calculator_widget_state_t *state,
                                            uint16_t x, uint16_t y);
const char *calculator_widget_tail(const char *text, size_t max_chars);

#endif
