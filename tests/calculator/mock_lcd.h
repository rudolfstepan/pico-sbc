#ifndef MOCK_LCD_H
#define MOCK_LCD_H

#include <stdbool.h>
#include <stdint.h>

void mock_lcd_reset(void);
bool mock_lcd_had_out_of_bounds_draw(void);
uint8_t mock_lcd_max_text_scale(void);
uint16_t mock_lcd_pixel(int x, int y);
bool mock_lcd_drew_text(const char *text);

#endif
