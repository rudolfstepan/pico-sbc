#pragma once

#include <stdint.h>

#define LCD_WIDTH  480
#define LCD_HEIGHT 320

#define RGB565(r, g, b) (uint16_t)((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) >> 3))

void lcd_init(void);
void lcd_set_backlight(uint8_t percent);
void lcd_fill(uint16_t color);
void lcd_fill_rect(int x, int y, int w, int h, uint16_t color);
void lcd_draw_rect(int x, int y, int w, int h, uint16_t color);
void lcd_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void lcd_draw_text(int x, int y, const char *text, uint16_t fg, uint16_t bg, uint8_t scale);
