#include "calculator_ui_input.h"

#include <stddef.h>

void calculator_touch_input_init(calculator_touch_input_t *input) {
    input->pressed_target = NULL;
    input->last_x = 0;
    input->last_y = 0;
    input->touching = false;
}

calculator_touch_event_t calculator_touch_input_update(
    calculator_touch_input_t *input, bool touched, uint16_t x, uint16_t y,
    const void *target, const void **event_target) {
    if (event_target) *event_target = NULL;
    if (touched) {
        input->last_x = x;
        input->last_y = y;
        if (!input->touching) {
            input->touching = true;
            input->pressed_target = target;
            if (event_target) *event_target = target;
            return target ? CALCULATOR_TOUCH_PRESSED : CALCULATOR_TOUCH_NONE;
        }
        if (input->pressed_target && target != input->pressed_target) {
            if (event_target) *event_target = input->pressed_target;
            input->pressed_target = NULL;
            return CALCULATOR_TOUCH_CANCELLED;
        }
        return CALCULATOR_TOUCH_NONE;
    }

    if (!input->touching) return CALCULATOR_TOUCH_NONE;
    input->touching = false;
    if (!input->pressed_target) return CALCULATOR_TOUCH_NONE;
    if (event_target) *event_target = input->pressed_target;
    input->pressed_target = NULL;
    return CALCULATOR_TOUCH_ACTIVATED;
}
