#include "mock_lcd.h"

#include "lcd_st7796.h"

#include <stddef.h>
#include <string.h>

static uint16_t framebuffer[LCD_PORTRAIT_HEIGHT][LCD_LANDSCAPE_WIDTH];
static bool out_of_bounds_draw;
static uint8_t max_text_scale;
static char text_log[4096];
static lcd_orientation_t current_orientation;

void mock_lcd_reset(void) {
    memset(framebuffer, 0, sizeof framebuffer);
    out_of_bounds_draw = false;
    max_text_scale = 0;
    text_log[0] = '\0';
}

bool mock_lcd_had_out_of_bounds_draw(void) {
    return out_of_bounds_draw;
}

uint8_t mock_lcd_max_text_scale(void) {
    return max_text_scale;
}

uint16_t mock_lcd_pixel(int x, int y) {
    if (x < 0 || x >= lcd_width() || y < 0 || y >= lcd_height()) return 0;
    return framebuffer[y][x];
}

bool mock_lcd_drew_text(const char *text) {
    return text && strstr(text_log, text) != NULL;
}

void lcd_init(void) {}
void lcd_set_orientation(lcd_orientation_t orientation) {
    current_orientation = orientation == LCD_ORIENTATION_PORTRAIT
        ? LCD_ORIENTATION_PORTRAIT : LCD_ORIENTATION_LANDSCAPE;
}
lcd_orientation_t lcd_orientation(void) { return current_orientation; }
int lcd_width(void) {
    return current_orientation == LCD_ORIENTATION_PORTRAIT
        ? LCD_PORTRAIT_WIDTH : LCD_LANDSCAPE_WIDTH;
}
int lcd_height(void) {
    return current_orientation == LCD_ORIENTATION_PORTRAIT
        ? LCD_PORTRAIT_HEIGHT : LCD_LANDSCAPE_HEIGHT;
}
void lcd_set_backlight(uint8_t percent) { (void)percent; }

void lcd_fill(uint16_t color) {
    for (int y = 0; y < lcd_height(); ++y) {
        for (int x = 0; x < lcd_width(); ++x) framebuffer[y][x] = color;
    }
}

void lcd_fill_rect(int x, int y, int width, int height, uint16_t color) {
    if (width <= 0 || height <= 0) return;
    if (x < 0 || y < 0 || x + width > lcd_width() ||
        y + height > lcd_height()) {
        out_of_bounds_draw = true;
    }
    int left = x < 0 ? 0 : x;
    int top = y < 0 ? 0 : y;
    int right = x + width > lcd_width() ? lcd_width() : x + width;
    int bottom = y + height > lcd_height() ? lcd_height() : y + height;
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
    if (x < 0 || y < 0 || x + 6 * scale > lcd_width() ||
        y + 8 * scale > lcd_height()) {
        out_of_bounds_draw = true;
    }
}

void lcd_draw_text(int x, int y, const char *text,
                   uint16_t foreground, uint16_t background, uint8_t scale) {
    size_t length = strlen(text);
    (void)foreground;
    (void)background;
    if (scale > max_text_scale) max_text_scale = scale;
    size_t log_length = strlen(text_log);
    if (log_length + length + 2u < sizeof text_log) {
        memcpy(text_log + log_length, text, length);
        text_log[log_length + length] = '\n';
        text_log[log_length + length + 1u] = '\0';
    }
    if (x < 0 || y < 0 ||
        x + (int)(length * 6u * scale) > lcd_width() ||
        y + 8 * scale > lcd_height()) {
        out_of_bounds_draw = true;
    }
}
