#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool touched;
    bool updated;
    uint16_t x;
    uint16_t y;
} touch_point_t;

bool touch_init(void);
bool touch_is_ready(void);
bool touch_read(touch_point_t *point);
