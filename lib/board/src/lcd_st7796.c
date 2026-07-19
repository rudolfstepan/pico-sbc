#include "lcd_st7796.h"

#include "board.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#include <ctype.h>
#include <string.h>

#define LCD_SPI spi0
#define LCD_SPI_BAUD (62500000u)

static const uint8_t font_digits[10][5] = {
    {0x3e,0x51,0x49,0x45,0x3e}, {0x00,0x42,0x7f,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4b,0x31},
    {0x18,0x14,0x12,0x7f,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3c,0x4a,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1e},
};

static const uint8_t font_upper[26][5] = {
    {0x7e,0x11,0x11,0x11,0x7e}, {0x7f,0x49,0x49,0x49,0x36},
    {0x3e,0x41,0x41,0x41,0x22}, {0x7f,0x41,0x41,0x22,0x1c},
    {0x7f,0x49,0x49,0x49,0x41}, {0x7f,0x09,0x09,0x09,0x01},
    {0x3e,0x41,0x49,0x49,0x7a}, {0x7f,0x08,0x08,0x08,0x7f},
    {0x00,0x41,0x7f,0x41,0x00}, {0x20,0x40,0x41,0x3f,0x01},
    {0x7f,0x08,0x14,0x22,0x41}, {0x7f,0x40,0x40,0x40,0x40},
    {0x7f,0x02,0x0c,0x02,0x7f}, {0x7f,0x04,0x08,0x10,0x7f},
    {0x3e,0x41,0x41,0x41,0x3e}, {0x7f,0x09,0x09,0x09,0x06},
    {0x3e,0x41,0x51,0x21,0x5e}, {0x7f,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31}, {0x01,0x01,0x7f,0x01,0x01},
    {0x3f,0x40,0x40,0x40,0x3f}, {0x1f,0x20,0x40,0x20,0x1f},
    {0x7f,0x20,0x18,0x20,0x7f}, {0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07}, {0x61,0x51,0x49,0x45,0x43},
};

static inline void lcd_select(void) {
    gpio_put(PIN_LCD_CS, 0);
}

static inline void lcd_deselect(void) {
    gpio_put(PIN_LCD_CS, 1);
}

static void lcd_write_cmd(uint8_t cmd) {
    lcd_select();
    gpio_put(PIN_LCD_DC, 0);
    spi_write_blocking(LCD_SPI, &cmd, 1);
    lcd_deselect();
}

static void lcd_write_data(const uint8_t *data, size_t len) {
    lcd_select();
    gpio_put(PIN_LCD_DC, 1);
    spi_write_blocking(LCD_SPI, data, len);
    lcd_deselect();
}

static void lcd_write_u8(uint8_t value) {
    lcd_write_data(&value, 1);
}

static void lcd_reset(void) {
    gpio_put(PIN_LCD_RST, 0);
    sleep_ms(40);
    gpio_put(PIN_LCD_RST, 1);
    sleep_ms(120);
}

static void lcd_set_window(int x0, int y0, int x1, int y1) {
    uint8_t data[4];
    lcd_write_cmd(0x2a);
    data[0] = (uint8_t)(x0 >> 8);
    data[1] = (uint8_t)x0;
    data[2] = (uint8_t)(x1 >> 8);
    data[3] = (uint8_t)x1;
    lcd_write_data(data, sizeof data);

    lcd_write_cmd(0x2b);
    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)y0;
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)y1;
    lcd_write_data(data, sizeof data);

    lcd_write_cmd(0x2c);
}

static const uint8_t *font_for(char c) {
    static const uint8_t blank[5] = {0,0,0,0,0};
    static const uint8_t minus[5] = {0x08,0x08,0x08,0x08,0x08};
    static const uint8_t plus[5] = {0x08,0x08,0x3e,0x08,0x08};
    static const uint8_t star[5] = {0x14,0x08,0x3e,0x08,0x14};
    static const uint8_t caret[5] = {0x04,0x02,0x01,0x02,0x04};
    static const uint8_t slash[5] = {0x20,0x10,0x08,0x04,0x02};
    static const uint8_t colon[5] = {0x00,0x36,0x36,0x00,0x00};
    static const uint8_t dot[5] = {0x00,0x60,0x60,0x00,0x00};
    static const uint8_t comma[5] = {0x00,0x40,0x30,0x00,0x00};
    static const uint8_t equals[5] = {0x14,0x14,0x14,0x14,0x14};
    static const uint8_t percent[5] = {0x63,0x13,0x08,0x64,0x63};
    static const uint8_t left_paren[5] = {0x00,0x1c,0x22,0x41,0x00};
    static const uint8_t right_paren[5] = {0x00,0x41,0x22,0x1c,0x00};
    static const uint8_t underscore[5] = {0x40,0x40,0x40,0x40,0x40};
    static const uint8_t lt[5] = {0x08,0x14,0x22,0x41,0x00};
    static const uint8_t gt[5] = {0x00,0x41,0x22,0x14,0x08};
    static const uint8_t bang[5] = {0x00,0x00,0x5f,0x00,0x00};

    c = (char)toupper((unsigned char)c);
    if (c >= '0' && c <= '9') return font_digits[c - '0'];
    if (c >= 'A' && c <= 'Z') return font_upper[c - 'A'];
    if (c == '-') return minus;
    if (c == '+') return plus;
    if (c == '*') return star;
    if (c == '^') return caret;
    if (c == '/') return slash;
    if (c == ':') return colon;
    if (c == '.') return dot;
    if (c == ',') return comma;
    if (c == '=') return equals;
    if (c == '%') return percent;
    if (c == '(') return left_paren;
    if (c == ')') return right_paren;
    if (c == '_') return underscore;
    if (c == '<') return lt;
    if (c == '>') return gt;
    if (c == '!') return bang;
    return blank;
}

void lcd_init(void) {
    spi_init(LCD_SPI, LCD_SPI_BAUD);
    gpio_set_function(PIN_LCD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_LCD_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_LCD_MISO, GPIO_FUNC_SPI);

    gpio_init(PIN_LCD_CS);
    gpio_set_dir(PIN_LCD_CS, GPIO_OUT);
    gpio_put(PIN_LCD_CS, 1);

    gpio_init(PIN_LCD_DC);
    gpio_set_dir(PIN_LCD_DC, GPIO_OUT);
    gpio_init(PIN_LCD_RST);
    gpio_set_dir(PIN_LCD_RST, GPIO_OUT);

    lcd_reset();

    lcd_write_cmd(0x01);
    sleep_ms(120);
    lcd_write_cmd(0x11);
    sleep_ms(120);
    lcd_write_cmd(0x36);
    lcd_write_u8(0x28); /* landscape, RGB order */
    lcd_write_cmd(0x3a);
    lcd_write_u8(0x55); /* RGB565 */
    lcd_write_cmd(0xb2);
    const uint8_t porch[] = {0x0c,0x0c,0x00,0x33,0x33};
    lcd_write_data(porch, sizeof porch);
    lcd_write_cmd(0xb7);
    lcd_write_u8(0x35);
    lcd_write_cmd(0xbb);
    lcd_write_u8(0x28);
    lcd_write_cmd(0xc0);
    lcd_write_u8(0x2c);
    lcd_write_cmd(0xc2);
    lcd_write_u8(0x01);
    lcd_write_cmd(0xc3);
    lcd_write_u8(0x0b);
    lcd_write_cmd(0xc4);
    lcd_write_u8(0x20);
    lcd_write_cmd(0xc6);
    lcd_write_u8(0x0f);
    lcd_write_cmd(0xd0);
    const uint8_t power[] = {0xa4,0xa1};
    lcd_write_data(power, sizeof power);
    lcd_write_cmd(0x21);
    lcd_write_cmd(0x29);
    sleep_ms(20);
}

void lcd_set_backlight(uint8_t percent) {
    (void)percent;
}

void lcd_fill(uint16_t color) {
    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, color);
}

void lcd_fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
    if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    uint8_t pair[2] = {(uint8_t)(color >> 8), (uint8_t)color};
    uint8_t line[128];
    for (size_t i = 0; i < sizeof line; i += 2) {
        line[i] = pair[0];
        line[i + 1] = pair[1];
    }

    lcd_set_window(x, y, x + w - 1, y + h - 1);
    lcd_select();
    gpio_put(PIN_LCD_DC, 1);
    int pixels = w * h;
    while (pixels > 0) {
        int chunk = pixels > 64 ? 64 : pixels;
        spi_write_blocking(LCD_SPI, line, (size_t)chunk * 2u);
        pixels -= chunk;
    }
    lcd_deselect();
}

void lcd_draw_rect(int x, int y, int w, int h, uint16_t color) {
    lcd_fill_rect(x, y, w, 1, color);
    lcd_fill_rect(x, y + h - 1, w, 1, color);
    lcd_fill_rect(x, y, 1, h, color);
    lcd_fill_rect(x + w - 1, y, 1, h, color);
}

void lcd_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, uint8_t scale) {
    const uint8_t *glyph = font_for(c);
    for (int col = 0; col < 6; ++col) {
        uint8_t bits = col < 5 ? glyph[col] : 0;
        for (int row = 0; row < 8; ++row) {
            uint16_t color = (bits & (1u << row)) ? fg : bg;
            lcd_fill_rect(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

void lcd_draw_text(int x, int y, const char *text, uint16_t fg, uint16_t bg, uint8_t scale) {
    while (*text) {
        lcd_draw_char(x, y, *text++, fg, bg, scale);
        x += 6 * scale;
        if (x > LCD_WIDTH - 6 * scale) {
            break;
        }
    }
}
