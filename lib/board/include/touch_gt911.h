#pragma once

#include "lcd_st7796.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool touched;
    bool updated;
    uint16_t x;
    uint16_t y;
} touch_point_t;

static inline bool touch_map_raw_coordinates(uint16_t raw_x, uint16_t raw_y,
                                             lcd_orientation_t orientation,
                                             uint16_t *x, uint16_t *y) {
    if (!x || !y || raw_x >= LCD_PORTRAIT_WIDTH ||
        raw_y >= LCD_PORTRAIT_HEIGHT) {
        return false;
    }
    if (orientation == LCD_ORIENTATION_PORTRAIT) {
        *x = raw_x;
        *y = raw_y;
    } else {
        *x = raw_y;
        *y = (uint16_t)(LCD_LANDSCAPE_HEIGHT - 1u - raw_x);
    }
    return true;
}

bool touch_init(void);
bool touch_is_ready(void);
bool touch_read(touch_point_t *point);
