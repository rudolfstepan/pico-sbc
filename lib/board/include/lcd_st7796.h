#pragma once

#include <stdint.h>

#define LCD_LANDSCAPE_WIDTH  480
#define LCD_LANDSCAPE_HEIGHT 320
#define LCD_PORTRAIT_WIDTH    320
#define LCD_PORTRAIT_HEIGHT   480

/* Legacy compile-time dimensions for fixed-size buffers. */
#define LCD_WIDTH  LCD_LANDSCAPE_WIDTH
#define LCD_HEIGHT LCD_LANDSCAPE_HEIGHT

#define RGB565(r, g, b) (uint16_t)((((r) & 0xf8) << 8) | (((g) & 0xfc) << 3) | ((b) >> 3))

typedef enum {
    LCD_ORIENTATION_LANDSCAPE,
    LCD_ORIENTATION_PORTRAIT
} lcd_orientation_t;

void lcd_init(void);
void lcd_set_orientation(lcd_orientation_t orientation);
lcd_orientation_t lcd_orientation(void);
int lcd_width(void);
int lcd_height(void);
void lcd_set_backlight(uint8_t percent);
void lcd_fill(uint16_t color);
void lcd_fill_rect(int x, int y, int w, int h, uint16_t color);
void lcd_draw_rect(int x, int y, int w, int h, uint16_t color);
void lcd_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, uint8_t scale);
void lcd_draw_text(int x, int y, const char *text, uint16_t fg, uint16_t bg, uint8_t scale);
