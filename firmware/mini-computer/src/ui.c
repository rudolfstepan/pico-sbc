#include "ui.h"

#include "board.h"
#include "lcd_st7796.h"
#include "touch_gt911.h"
#include "pico/stdlib.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define COL_BG       RGB565(0, 0, 0)
#define COL_PANEL    RGB565(64, 64, 64)
#define COL_KEY      RGB565(238, 238, 238)
#define COL_SPECIAL  RGB565(255, 255, 255)
#define COL_DARK_KEY RGB565(0, 0, 0)
#define COL_TEXT     RGB565(255, 255, 255)
#define COL_KEY_TEXT RGB565(0, 0, 0)
#define COL_MUTED    RGB565(185, 185, 185)
#define COL_BORDER   RGB565(255, 255, 255)

#define TERM_X 0
#define TERM_Y 0
#define TERM_W 480
#define TERM_H 104
#define ROW_H 18
#define MAX_LINES 5
#define MAX_LINE 38
#define CMD_MAX 32

typedef struct {
    int x, y, w, h;
    const char *label;
    char value;
} ui_key_t;

static char lines[MAX_LINES][MAX_LINE];
static uint8_t line_count;
static char command[CMD_MAX];
static uint8_t command_len;
static uint32_t last_touch_ms;

static const ui_key_t keys[] = {
    {8, 140, 42, 30, "1", '1'}, {54, 140, 42, 30, "2", '2'}, {100, 140, 42, 30, "3", '3'}, {146, 140, 42, 30, "4", '4'}, {192, 140, 42, 30, "5", '5'},
    {238, 140, 42, 30, "6", '6'}, {284, 140, 42, 30, "7", '7'}, {330, 140, 42, 30, "8", '8'}, {376, 140, 42, 30, "9", '9'}, {422, 140, 42, 30, "0", '0'},
    {8, 172, 42, 30, "Q", 'Q'}, {54, 172, 42, 30, "W", 'W'}, {100, 172, 42, 30, "E", 'E'}, {146, 172, 42, 30, "R", 'R'}, {192, 172, 42, 30, "T", 'T'},
    {238, 172, 42, 30, "Z", 'Z'}, {284, 172, 42, 30, "U", 'U'}, {330, 172, 42, 30, "I", 'I'}, {376, 172, 42, 30, "O", 'O'}, {422, 172, 42, 30, "P", 'P'},
    {20, 204, 38, 30, "A", 'A'}, {62, 204, 38, 30, "S", 'S'}, {104, 204, 38, 30, "D", 'D'}, {146, 204, 38, 30, "F", 'F'}, {188, 204, 38, 30, "G", 'G'},
    {230, 204, 38, 30, "H", 'H'}, {272, 204, 38, 30, "J", 'J'}, {314, 204, 38, 30, "K", 'K'}, {356, 204, 38, 30, "L", 'L'},
    {8, 236, 48, 30, "DEL", '\b'}, {60, 236, 44, 30, "Y", 'Y'}, {108, 236, 44, 30, "X", 'X'}, {156, 236, 44, 30, "C", 'C'},
    {204, 236, 44, 30, "V", 'V'}, {252, 236, 44, 30, "B", 'B'}, {300, 236, 44, 30, "N", 'N'}, {348, 236, 44, 30, "M", 'M'},
    {400, 204, 72, 62, "ENTER", '\n'},
    {8, 268, 72, 28, "CLEAR", 0x7f}, {84, 268, 48, 28, "-", '-'}, {136, 268, 208, 28, "SPACE", ' '},
    {348, 268, 48, 28, "/", '/'}, {400, 268, 72, 28, "BEEP", 0x01},
};

static void push_line(const char *text) {
    if (line_count == MAX_LINES) {
        memmove(lines[0], lines[1], (MAX_LINES - 1) * MAX_LINE);
        line_count--;
    }
    snprintf(lines[line_count], MAX_LINE, "%s", text);
    line_count++;
}

static void render_terminal(void) {
    lcd_fill_rect(TERM_X, TERM_Y, TERM_W, TERM_H, COL_BG);
    for (uint8_t i = 0; i < line_count; ++i) {
        lcd_draw_text(TERM_X + 6, TERM_Y + 6 + i * ROW_H, lines[i], COL_TEXT, COL_BG, 1);
    }
    lcd_fill_rect(0, 106, LCD_WIDTH, 28, COL_BG);
    char prompt[MAX_LINE];
    snprintf(prompt, sizeof prompt, "> %s_", command);
    lcd_draw_text(6, 115, prompt, COL_TEXT, COL_BG, 1);
}

static uint16_t key_color(const ui_key_t *key) {
    if (key->value == '\b' || key->value == 0x7f || key->value == 0x01) {
        return COL_DARK_KEY;
    }
    if (key->value == '\n' || key->value == ' ') {
        return COL_SPECIAL;
    }
    return COL_KEY;
}

static void draw_key(const ui_key_t *key, bool pressed) {
    uint16_t fill = pressed ? COL_BG : key_color(key);
    uint16_t text = pressed ? COL_TEXT :
        (fill == COL_DARK_KEY ? COL_TEXT : COL_KEY_TEXT);
    uint8_t scale = 1;
    int text_w = (int)strlen(key->label) * 6 * scale;
    int text_h = 8 * scale;
    int tx = key->x + (key->w - text_w) / 2;
    int ty = key->y + (key->h - text_h) / 2;

    lcd_fill_rect(key->x, key->y, key->w, key->h, fill);
    lcd_draw_rect(key->x, key->y, key->w, key->h,
                  fill == COL_DARK_KEY ? COL_BORDER : COL_BG);
    lcd_draw_rect(key->x + 1, key->y + 1, key->w - 2, key->h - 2,
                  fill == COL_DARK_KEY ? COL_BORDER : COL_BG);
    lcd_draw_text(tx, ty, key->label, text, fill, scale);
}

static void render_keyboard(void) {
    lcd_fill_rect(0, 136, LCD_WIDTH, 184, COL_PANEL);
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
        draw_key(&keys[i], false);
    }
}

static void execute_command(void) {
    char buf[MAX_LINE];
    for (uint8_t i = 0; command[i]; ++i) {
        command[i] = (char)toupper((unsigned char)command[i]);
    }
    snprintf(buf, sizeof buf, "> %s", command);
    push_line(buf);

    if (command_len == 0) {
        push_line("READY");
    } else if (strcmp(command, "HELP") == 0) {
        push_line("CMDS: HELP INFO CLS BEEP");
        push_line("LED1 ON/OFF  LED2 ON/OFF");
    } else if (strcmp(command, "INFO") == 0) {
        push_line("RP2040 320X480 ST7796U GT911");
        push_line("TOUCH UI UART USB STDIO");
    } else if (strcmp(command, "CLS") == 0) {
        line_count = 0;
    } else if (strcmp(command, "BEEP") == 0) {
        push_line("BEEP");
        board_beep(1200, 80);
    } else if (strcmp(command, "LED1 ON") == 0) {
        board_led1(true);
        push_line("LED1 ON");
    } else if (strcmp(command, "LED1 OFF") == 0) {
        board_led1(false);
        push_line("LED1 OFF");
    } else if (strcmp(command, "LED2 ON") == 0) {
        board_led2(true);
        push_line("LED2 ON");
    } else if (strcmp(command, "LED2 OFF") == 0) {
        board_led2(false);
        push_line("LED2 OFF");
    } else {
        push_line("UNKNOWN COMMAND");
    }

    command_len = 0;
    command[0] = '\0';
}

static void input_char(char c) {
    if (c == '\n') {
        execute_command();
    } else if (c == '\b') {
        if (command_len > 0) {
            command[--command_len] = '\0';
        }
    } else if (c == 0x7f) {
        command_len = 0;
        command[0] = '\0';
    } else if (c == 0x01) {
        board_beep(1000, 40);
    } else if (command_len + 1 < CMD_MAX) {
        command[command_len++] = c;
        command[command_len] = '\0';
    }
    render_terminal();
}

static bool hit_key(uint16_t x, uint16_t y, char *value) {
    for (size_t i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
        if (x >= keys[i].x && x < keys[i].x + keys[i].w &&
            y >= keys[i].y && y < keys[i].y + keys[i].h) {
            *value = keys[i].value;
            draw_key(&keys[i], true);
            sleep_ms(25);
            draw_key(&keys[i], false);
            return true;
        }
    }
    return false;
}

void ui_init(void) {
    lcd_fill(COL_BG);
    render_terminal();
    render_keyboard();
}

void ui_task(void) {
    touch_point_t point;
    if (touch_read(&point) && point.touched) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_touch_ms > 160) {
            char value;
            if (hit_key(point.x, point.y, &value)) {
                input_char(value);
                board_beep(1800, 12);
            }
            last_touch_ms = now;
        }
    }

    static bool old_b1 = false;
    static bool old_b2 = false;
    bool b1 = board_button1_pressed();
    bool b2 = board_button2_pressed();
    if (b1 && !old_b1) input_char('\n');
    if (b2 && !old_b2) input_char(0x7f);
    old_b1 = b1;
    old_b2 = b2;
}
