#ifndef CALCULATOR_UI_INPUT_H
#define CALCULATOR_UI_INPUT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CALCULATOR_TOUCH_NONE,
    CALCULATOR_TOUCH_PRESSED,
    CALCULATOR_TOUCH_CANCELLED,
    CALCULATOR_TOUCH_ACTIVATED
} calculator_touch_event_t;

typedef struct {
    const void *pressed_target;
    uint16_t last_x;
    uint16_t last_y;
    bool touching;
} calculator_touch_input_t;

void calculator_touch_input_init(calculator_touch_input_t *input);
calculator_touch_event_t calculator_touch_input_update(
    calculator_touch_input_t *input, bool touched, uint16_t x, uint16_t y,
    const void *target, const void **event_target);

#endif
