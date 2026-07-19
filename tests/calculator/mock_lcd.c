#include "mock_lcd.h"

#include "lcd_st7796.h"

#include <stddef.h>
#include <string.h>

static uint16_t framebuffer[LCD_HEIGHT][LCD_WIDTH];
static bool out_of_bounds_draw;
static uint8_t max_text_scale;

void mock_lcd_reset(void) {
    memset(framebuffer, 0, sizeof framebuffer);
    out_of_bounds_draw = false;
    max_text_scale = 0;
}

bool mock_lcd_had_out_of_bounds_draw(void) {
    return out_of_bounds_draw;
}

uint8_t mock_lcd_max_text_scale(void) {
    return max_text_scale;
}

uint16_t mock_lcd_pixel(int x, int y) {
    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) return 0;
    return framebuffer[y][x];
}

void lcd_init(void) {}
void lcd_set_backlight(uint8_t percent) { (void)percent; }

void lcd_fill(uint16_t color) {
    for (int y = 0; y < LCD_HEIGHT; ++y) {
        for (int x = 0; x < LCD_WIDTH; ++x) framebuffer[y][x] = color;
    }
}

void lcd_fill_rect(int x, int y, int width, int height, uint16_t color) {
    if (width <= 0 || height <= 0) return;
    if (x < 0 || y < 0 || x + width > LCD_WIDTH || y + height > LCD_HEIGHT) {
        out_of_bounds_draw = true;
    }
    int left = x < 0 ? 0 : x;
    int top = y < 0 ? 0 : y;
    int right = x + width > LCD_WIDTH ? LCD_WIDTH : x + width;
    int bottom = y + height > LCD_HEIGHT ? LCD_HEIGHT : y + height;
    for (int row = top; row < bottom; ++row) {
        for (int column = left; column < right; ++column) {
            framebuffer[row][column] = color;
        }
    }
}

void lcd_draw_rect(int x, int y, int width, int height, uint16_t color) {
    lcd_fill_rect(x, y, width, 1, color);
    lcd_fill_rect(x, y + height - 1, width, 1, color);
    lcd_fill_rect(x, y, 1, height, color);
    lcd_fill_rect(x + width - 1, y, 1, height, color);
}

void lcd_draw_char(int x, int y, char character,
                   uint16_t foreground, uint16_t background, uint8_t scale) {
    (void)character;
    (void)foreground;
    (void)background;
    if (scale > max_text_scale) max_text_scale = scale;
    if (x < 0 || y < 0 || x + 6 * scale > LCD_WIDTH ||
        y + 8 * scale > LCD_HEIGHT) {
        out_of_bounds_draw = true;
    }
}

void lcd_draw_text(int x, int y, const char *text,
                   uint16_t foreground, uint16_t background, uint8_t scale) {
    size_t length = strlen(text);
    (void)foreground;
    (void)background;
    if (scale > max_text_scale) max_text_scale = scale;
    if (x < 0 || y < 0 || x + (int)(length * 6u * scale) > LCD_WIDTH ||
        y + 8 * scale > LCD_HEIGHT) {
        out_of_bounds_draw = true;
    }
}
