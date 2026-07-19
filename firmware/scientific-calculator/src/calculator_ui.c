#include "calculator_ui.h"

#include "calculator_engine.h"
#include "number_formats.h"
#include "programmer_engine.h"
#include "board.h"
#include "lcd_st7796.h"
#include "touch_gt911.h"
#include "pico/stdlib.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define COL_BG       RGB565(0, 0, 0)
#define COL_TEXT     RGB565(255, 255, 255)
#define COL_MUTED    RGB565(170, 170, 170)
#define COL_GRID     RGB565(48, 48, 48)
#define COL_KEY      RGB565(238, 238, 238)
#define COL_FUNCTION RGB565(170, 170, 170)
#define COL_COMMAND  RGB565(60, 60, 60)

#define DISPLAY_H 84
#define KEY_X 4
#define KEY_Y 88
#define KEY_W 75
#define KEY_H 42
#define KEY_GAP_X 4
#define KEY_GAP_Y 4
#define EXPR_MAX 96

typedef enum {
    ACT_INSERT,
    ACT_EVALUATE,
    ACT_CLEAR,
    ACT_DELETE,
    ACT_PAGE,
    ACT_ANGLE,
    ACT_PROG_DIGIT,
    ACT_PROG_BASE,
    ACT_PROG_BINARY,
    ACT_PROG_UNARY,
    ACT_GOTO_PROGRAMMER,
    ACT_FMT_WIDTH,
    ACT_FMT_ACTION,
    ACT_FMT_GOTO_BASE,
    ACT_GOTO_GRAPH,
    ACT_GOTO_TOOLS,
    ACT_MEMORY,
    ACT_HISTORY,
    ACT_CURSOR,
    ACT_GRAPH
} calc_action_t;

typedef enum {
    PAGE_BASIC,
    PAGE_SCIENTIFIC,
    PAGE_PROGRAMMER,
    PAGE_FORMAT,
    PAGE_TOOLS,
    PAGE_GRAPH
} calc_page_t;

typedef enum {
    STYLE_NUMBER,
    STYLE_FUNCTION,
    STYLE_COMMAND,
    STYLE_EQUALS
} key_style_t;

typedef struct {
    const char *label;
    const char *token;
    uint8_t col;
    uint8_t row;
    calc_action_t action;
    key_style_t style;
} calc_key_t;

static const calc_key_t basic_keys[] = {
    {"SCI", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"DEG", "", 1, 0, ACT_ANGLE, STYLE_COMMAND},
    {"(", "(", 2, 0, ACT_INSERT, STYLE_FUNCTION},
    {")", ")", 3, 0, ACT_INSERT, STYLE_FUNCTION},
    {"DEL", "", 4, 0, ACT_DELETE, STYLE_COMMAND},
    {"AC", "", 5, 0, ACT_CLEAR, STYLE_COMMAND},

    {"7", "7", 0, 1, ACT_INSERT, STYLE_NUMBER},
    {"8", "8", 1, 1, ACT_INSERT, STYLE_NUMBER},
    {"9", "9", 2, 1, ACT_INSERT, STYLE_NUMBER},
    {"/", "/", 3, 1, ACT_INSERT, STYLE_FUNCTION},
    {"SQRT", "sqrt(", 4, 1, ACT_INSERT, STYLE_FUNCTION},
    {"X^2", "^2", 5, 1, ACT_INSERT, STYLE_FUNCTION},

    {"4", "4", 0, 2, ACT_INSERT, STYLE_NUMBER},
    {"5", "5", 1, 2, ACT_INSERT, STYLE_NUMBER},
    {"6", "6", 2, 2, ACT_INSERT, STYLE_NUMBER},
    {"*", "*", 3, 2, ACT_INSERT, STYLE_FUNCTION},
    {"1/X", "1/(", 4, 2, ACT_INSERT, STYLE_FUNCTION},
    {"X^Y", "^", 5, 2, ACT_INSERT, STYLE_FUNCTION},

    {"1", "1", 0, 3, ACT_INSERT, STYLE_NUMBER},
    {"2", "2", 1, 3, ACT_INSERT, STYLE_NUMBER},
    {"3", "3", 2, 3, ACT_INSERT, STYLE_NUMBER},
    {"-", "-", 3, 3, ACT_INSERT, STYLE_FUNCTION},
    {"PI", "pi", 4, 3, ACT_INSERT, STYLE_FUNCTION},
    {"%", "%", 5, 3, ACT_INSERT, STYLE_FUNCTION},

    {"0", "0", 0, 4, ACT_INSERT, STYLE_NUMBER},
    {".", ".", 1, 4, ACT_INSERT, STYLE_NUMBER},
    {"ANS", "ans", 2, 4, ACT_INSERT, STYLE_FUNCTION},
    {"+", "+", 3, 4, ACT_INSERT, STYLE_FUNCTION},
    {",", ",", 4, 4, ACT_INSERT, STYLE_FUNCTION},
    {"=", "", 5, 4, ACT_EVALUATE, STYLE_EQUALS},
};

static const calc_key_t scientific_keys[] = {
    {"PROG", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"DEG", "", 1, 0, ACT_ANGLE, STYLE_COMMAND},
    {"(", "(", 2, 0, ACT_INSERT, STYLE_FUNCTION},
    {")", ")", 3, 0, ACT_INSERT, STYLE_FUNCTION},
    {"DEL", "", 4, 0, ACT_DELETE, STYLE_COMMAND},
    {"AC", "", 5, 0, ACT_CLEAR, STYLE_COMMAND},

    {"SIN", "sin(", 0, 1, ACT_INSERT, STYLE_FUNCTION},
    {"COS", "cos(", 1, 1, ACT_INSERT, STYLE_FUNCTION},
    {"TAN", "tan(", 2, 1, ACT_INSERT, STYLE_FUNCTION},
    {"ASIN", "asin(", 3, 1, ACT_INSERT, STYLE_FUNCTION},
    {"ACOS", "acos(", 4, 1, ACT_INSERT, STYLE_FUNCTION},
    {"ATAN", "atan(", 5, 1, ACT_INSERT, STYLE_FUNCTION},

    {"LN", "ln(", 0, 2, ACT_INSERT, STYLE_FUNCTION},
    {"LOG", "log(", 1, 2, ACT_INSERT, STYLE_FUNCTION},
    {"SQRT", "sqrt(", 2, 2, ACT_INSERT, STYLE_FUNCTION},
    {"EXP", "exp(", 3, 2, ACT_INSERT, STYLE_FUNCTION},
    {"10^X", "10^(", 4, 2, ACT_INSERT, STYLE_FUNCTION},
    {"X^Y", "^", 5, 2, ACT_INSERT, STYLE_FUNCTION},

    {"SINH", "sinh(", 0, 3, ACT_INSERT, STYLE_FUNCTION},
    {"COSH", "cosh(", 1, 3, ACT_INSERT, STYLE_FUNCTION},
    {"TANH", "tanh(", 2, 3, ACT_INSERT, STYLE_FUNCTION},
    {"ABS", "abs(", 3, 3, ACT_INSERT, STYLE_FUNCTION},
    {"FLOOR", "floor(", 4, 3, ACT_INSERT, STYLE_FUNCTION},
    {",", ",", 5, 3, ACT_INSERT, STYLE_FUNCTION},

    {"PI", "pi", 0, 4, ACT_INSERT, STYLE_FUNCTION},
    {"E", "e", 1, 4, ACT_INSERT, STYLE_FUNCTION},
    {"FAC", "fac(", 2, 4, ACT_INSERT, STYLE_FUNCTION},
    {"NCR", "ncr(", 3, 4, ACT_INSERT, STYLE_FUNCTION},
    {"NPR", "npr(", 4, 4, ACT_INSERT, STYLE_FUNCTION},
    {"=", "", 5, 4, ACT_EVALUATE, STYLE_EQUALS},
};

static const calc_key_t programmer_keys[] = {
    {"FMT", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"DEC", "DEC", 1, 0, ACT_PROG_BASE, STYLE_COMMAND},
    {"HEX", "HEX", 2, 0, ACT_PROG_BASE, STYLE_COMMAND},
    {"BIN", "BIN", 3, 0, ACT_PROG_BASE, STYLE_COMMAND},
    {"DEL", "", 4, 0, ACT_DELETE, STYLE_COMMAND},
    {"AC", "", 5, 0, ACT_CLEAR, STYLE_COMMAND},

    {"A", "A", 0, 1, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"B", "B", 1, 1, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"C", "C", 2, 1, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"D", "D", 3, 1, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"E", "E", 4, 1, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"F", "F", 5, 1, ACT_PROG_DIGIT, STYLE_NUMBER},

    {"7", "7", 0, 2, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"8", "8", 1, 2, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"9", "9", 2, 2, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"AND", "AND", 3, 2, ACT_PROG_BINARY, STYLE_FUNCTION},
    {"OR", "OR", 4, 2, ACT_PROG_BINARY, STYLE_FUNCTION},
    {"XOR", "XOR", 5, 2, ACT_PROG_BINARY, STYLE_FUNCTION},

    {"4", "4", 0, 3, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"5", "5", 1, 3, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"6", "6", 2, 3, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"<<", "SHL", 3, 3, ACT_PROG_UNARY, STYLE_FUNCTION},
    {">>", "SHR", 4, 3, ACT_PROG_UNARY, STYLE_FUNCTION},
    {"NOT", "NOT", 5, 3, ACT_PROG_UNARY, STYLE_FUNCTION},

    {"1", "1", 0, 4, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"2", "2", 1, 4, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"3", "3", 2, 4, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"0", "0", 3, 4, ACT_PROG_DIGIT, STYLE_NUMBER},
    {"NEG", "NEG", 4, 4, ACT_PROG_UNARY, STYLE_FUNCTION},
    {"=", "", 5, 4, ACT_EVALUATE, STYLE_EQUALS},
};

static const calc_key_t format_keys[] = {
    {"TOOLS", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"PROG", "", 1, 0, ACT_GOTO_PROGRAMMER, STYLE_COMMAND},
    {"8BIT", "8", 2, 0, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"16BIT", "16", 3, 0, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"32BIT", "32", 4, 0, ACT_FMT_WIDTH, STYLE_COMMAND},
    {"64BIT", "64", 5, 0, ACT_FMT_WIDTH, STYLE_COMMAND},

    {"TCNEG", "NEG", 0, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"MASK", "MASK", 1, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"SEXT", "SEXT", 2, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"Q-", "Q-", 3, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"Q+", "Q+", 4, 1, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"Q0", "Q0", 5, 1, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"ANS>F32", "A32", 0, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ANS>F64", "A64", 1, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"F32>ANS", "32A", 2, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"F64>ANS", "64A", 3, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ZERO", "ZERO", 4, 2, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"ONES", "ONES", 5, 2, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"MIN", "MIN", 0, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"MAX", "MAX", 1, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"+1", "INC", 2, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"-1", "DEC", 3, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"Q8", "Q8", 4, 3, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"Q16", "Q16", 5, 3, ACT_FMT_ACTION, STYLE_FUNCTION},

    {"Q24", "Q24", 0, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
    {"DEC", "DEC", 1, 4, ACT_FMT_GOTO_BASE, STYLE_COMMAND},
    {"HEX", "HEX", 2, 4, ACT_FMT_GOTO_BASE, STYLE_COMMAND},
    {"BIN", "BIN", 3, 4, ACT_FMT_GOTO_BASE, STYLE_COMMAND},
    {"<<", "SHL", 4, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
    {">>", "SHR", 5, 4, ACT_FMT_ACTION, STYLE_FUNCTION},
};

static const calc_key_t tools_keys[] = {
    {"BASIC", "", 0, 0, ACT_PAGE, STYLE_COMMAND},
    {"GRAPH", "", 1, 0, ACT_GOTO_GRAPH, STYLE_COMMAND},
    {"M+", "M+", 2, 0, ACT_MEMORY, STYLE_FUNCTION},
    {"M-", "M-", 3, 0, ACT_MEMORY, STYLE_FUNCTION},
    {"MR", "MR", 4, 0, ACT_MEMORY, STYLE_FUNCTION},
    {"MC", "MC", 5, 0, ACT_MEMORY, STYLE_FUNCTION},

    {"PREV", "PREV", 0, 1, ACT_HISTORY, STYLE_FUNCTION},
    {"NEXT", "NEXT", 1, 1, ACT_HISTORY, STYLE_FUNCTION},
    {"USE", "USE", 2, 1, ACT_HISTORY, STYLE_FUNCTION},
    {"HCLR", "CLEAR", 3, 1, ACT_HISTORY, STYLE_FUNCTION},
    {"ANS", "ans", 4, 1, ACT_INSERT, STYLE_FUNCTION},
    {"X", "x", 5, 1, ACT_INSERT, STYLE_FUNCTION},

    {"<", "LEFT", 0, 2, ACT_CURSOR, STYLE_FUNCTION},
    {">", "RIGHT", 1, 2, ACT_CURSOR, STYLE_FUNCTION},
    {"HOME", "HOME", 2, 2, ACT_CURSOR, STYLE_FUNCTION},
    {"END", "END", 3, 2, ACT_CURSOR, STYLE_FUNCTION},
    {"DEL", "", 4, 2, ACT_DELETE, STYLE_COMMAND},
    {"AC", "", 5, 2, ACT_CLEAR, STYLE_COMMAND},
};

static const calc_key_t graph_keys[] = {
    {"TOOLS", "", 0, 4, ACT_GOTO_TOOLS, STYLE_COMMAND},
    {"PLOT", "PLOT", 1, 4, ACT_GRAPH, STYLE_EQUALS},
    {"X-", "LEFT", 2, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"X+", "RIGHT", 3, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"ZOOM+", "IN", 4, 4, ACT_GRAPH, STYLE_FUNCTION},
    {"ZOOM-", "OUT", 5, 4, ACT_GRAPH, STYLE_FUNCTION},
};

static char expression[EXPR_MAX];
static char result_text[32] = "0";
static char message[24] = "READY";
static size_t expression_len;
static size_t expression_cursor;
static double ans;
static calc_page_t page;
static programmer_engine_t programmer;
static unsigned int format_bits = 32;
static unsigned int fixed_fraction_bits = 16;
static bool just_evaluated;
static bool touch_locked;

#define HISTORY_MAX 8
typedef struct {
    char expression[EXPR_MAX];
    char result[32];
    double value;
} history_entry_t;

static history_entry_t history[HISTORY_MAX];
static size_t history_count;
static size_t history_index;
static double memory_value;
static double graph_x_center;
static double graph_y_center;
static double graph_x_span = 20.0;
static double graph_y_span = 10.0;

static const calc_key_t *active_keys(size_t *count) {
    switch (page) {
        case PAGE_SCIENTIFIC:
            *count = sizeof scientific_keys / sizeof scientific_keys[0];
            return scientific_keys;
        case PAGE_PROGRAMMER:
            *count = sizeof programmer_keys / sizeof programmer_keys[0];
            return programmer_keys;
        case PAGE_FORMAT:
            *count = sizeof format_keys / sizeof format_keys[0];
            return format_keys;
        case PAGE_TOOLS:
            *count = sizeof tools_keys / sizeof tools_keys[0];
            return tools_keys;
        case PAGE_GRAPH:
            *count = sizeof graph_keys / sizeof graph_keys[0];
            return graph_keys;
        case PAGE_BASIC:
        default:
            *count = sizeof basic_keys / sizeof basic_keys[0];
            return basic_keys;
    }
}

static int key_x(const calc_key_t *key) {
    return KEY_X + key->col * (KEY_W + KEY_GAP_X);
}

static int key_y(const calc_key_t *key) {
    return KEY_Y + key->row * (KEY_H + KEY_GAP_Y);
}

static uint16_t key_fill(const calc_key_t *key, bool pressed) {
    if (pressed) return COL_BG;
    if (page == PAGE_PROGRAMMER && key->action == ACT_PROG_BASE) {
        programmer_base_t key_base = strcmp(key->token, "BIN") == 0
            ? PROGRAMMER_BIN
            : (strcmp(key->token, "HEX") == 0 ? PROGRAMMER_HEX : PROGRAMMER_DEC);
        if (programmer.base == key_base) return COL_TEXT;
    }
    if (page == PAGE_FORMAT && key->action == ACT_FMT_WIDTH) {
        unsigned int key_bits = key->token[0] == '8' ? 8u :
            (key->token[0] == '1' ? 16u : (key->token[0] == '3' ? 32u : 64u));
        if (key_bits == format_bits) return COL_TEXT;
    }
    switch (key->style) {
        case STYLE_NUMBER: return COL_KEY;
        case STYLE_FUNCTION: return COL_FUNCTION;
        case STYLE_COMMAND: return COL_COMMAND;
        case STYLE_EQUALS: return COL_TEXT;
        default: return COL_KEY;
    }
}

static void draw_key(const calc_key_t *key, bool pressed) {
    int x = key_x(key);
    int y = key_y(key);
    const char *label = key->action == ACT_ANGLE
        ? (calc_engine_uses_degrees() ? "DEG" : "RAD")
        : key->label;
    uint16_t fill = key_fill(key, pressed);
    bool disabled = false;
    if (page == PAGE_PROGRAMMER && key->action == ACT_PROG_DIGIT) {
        int digit = key->token[0] <= '9'
            ? key->token[0] - '0'
            : key->token[0] - 'A' + 10;
        disabled = digit >= (int)programmer.base;
        if (disabled && !pressed) fill = COL_COMMAND;
    }
    bool dark = fill == COL_BG || fill == COL_COMMAND;
    uint16_t fg = disabled ? COL_MUTED : (dark ? COL_TEXT : COL_BG);
    uint16_t border = dark ? COL_TEXT : COL_BG;
    size_t label_len = strlen(label);
    int scale_x = (KEY_W - 8) / ((int)label_len * 6);
    int scale_y = (KEY_H - 8) / 8;
    uint8_t scale = (uint8_t)(scale_x < scale_y ? scale_x : scale_y);
    if (scale > 4) scale = 4;
    if (scale < 1) scale = 1;
    int text_w = (int)label_len * 6 * scale;
    int text_h = 8 * scale;

    lcd_fill_rect(x, y, KEY_W, KEY_H, fill);
    lcd_draw_rect(x, y, KEY_W, KEY_H, border);
    lcd_draw_rect(x + 1, y + 1, KEY_W - 2, KEY_H - 2, border);
    lcd_draw_text(x + (KEY_W - text_w) / 2, y + (KEY_H - text_h) / 2,
                  label, fg, fill, scale);
}

static const char *tail_for_display(const char *text, size_t max_chars) {
    size_t len = strlen(text);
    return len > max_chars ? text + len - max_chars : text;
}

static const char *expression_editor_view(char *editor, size_t max_chars) {
    if (expression_len) {
        memcpy(editor, expression, expression_cursor);
        editor[expression_cursor] = '_';
        memcpy(editor + expression_cursor + 1, expression + expression_cursor,
               expression_len - expression_cursor + 1);
    } else {
        snprintf(editor, EXPR_MAX + 2, "_");
    }

    size_t editor_len = strlen(editor);
    size_t editor_start = 0;
    if (editor_len > max_chars) {
        size_t half = max_chars / 2;
        editor_start = expression_cursor > half ? expression_cursor - half : 0;
        if (editor_start + max_chars > editor_len) {
            editor_start = editor_len - max_chars;
        }
    }
    return editor + editor_start;
}

static void render_programmer_display(void) {
    char status[48];
    char decimal[21];
    char hexadecimal[17];
    char binary[65];
    char conversions[48];
    char binary_line[72];

    programmer_engine_format(programmer.value, PROGRAMMER_DEC,
                             decimal, sizeof decimal);
    programmer_engine_format(programmer.value, PROGRAMMER_HEX,
                             hexadecimal, sizeof hexadecimal);
    programmer_engine_format(programmer.value, PROGRAMMER_BIN,
                             binary, sizeof binary);
    snprintf(status, sizeof status, "PROGRAMMER  %s  %s%s%s",
             programmer_engine_base_name(programmer.base),
             programmer_engine_op_name(programmer.pending_op),
             programmer.pending_op == PROGRAMMER_OP_NONE ? "" : "  ",
             message);
    snprintf(conversions, sizeof conversions, "DEC %s  HEX %s",
             decimal, hexadecimal);
    snprintf(binary_line, sizeof binary_line, "BIN %s", binary);

    lcd_fill_rect(0, 0, LCD_WIDTH, DISPLAY_H, COL_BG);
    lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 18, tail_for_display(programmer.input, 38),
                  COL_TEXT, COL_BG, 2);
    lcd_draw_text(6, 45, conversions, COL_TEXT, COL_BG, 1);
    lcd_draw_text(6, 62, binary_line, COL_MUTED, COL_BG, 1);
    lcd_fill_rect(0, DISPLAY_H - 2, LCD_WIDTH, 2, COL_MUTED);
}

static void render_format_display(void) {
    uint64_t raw = number_format_apply_width(programmer.value, format_bits);
    char hexadecimal[17];
    char signed_text[24];
    char status[48];
    char raw_line[24];
    char fixed_line[48];
    char float_line[64];

    programmer_engine_format(raw, PROGRAMMER_HEX,
                             hexadecimal, sizeof hexadecimal);
    number_format_signed_text(raw, format_bits,
                              signed_text, sizeof signed_text);
    double fixed = number_format_fixed_value(raw, format_bits,
                                             fixed_fraction_bits);
    double float32 = number_format_bits_float32((uint32_t)raw);
    double float64 = number_format_bits_float64(raw);

    snprintf(status, sizeof status, "FORMAT  %uBIT  Q%u  %s",
             format_bits, fixed_fraction_bits, message);
    snprintf(raw_line, sizeof raw_line, "RAW %s", hexadecimal);
    snprintf(fixed_line, sizeof fixed_line, "TC %s  FIX %.10g",
             signed_text, fixed);
    snprintf(float_line, sizeof float_line, "F32 %.8g  F64 %.10g",
             float32, float64);

    lcd_fill_rect(0, 0, LCD_WIDTH, DISPLAY_H, COL_BG);
    lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 18, raw_line, COL_TEXT, COL_BG, 2);
    lcd_draw_text(6, 47, fixed_line, COL_TEXT, COL_BG, 1);
    lcd_draw_text(6, 64, float_line, COL_MUTED, COL_BG, 1);
    lcd_fill_rect(0, DISPLAY_H - 2, LCD_WIDTH, 2, COL_MUTED);
}

static void render_tools_display(void) {
    char status[48];
    char editor[EXPR_MAX + 2];
    char history_line[80];
    char result_line[48];

    snprintf(status, sizeof status, "TOOLS  M %.10g  %s", memory_value, message);
    if (history_count) {
        snprintf(history_line, sizeof history_line, "H%u %s",
                 (unsigned int)(history_index + 1),
                 history[history_index].expression);
        snprintf(result_line, sizeof result_line, "RESULT %s",
                 history[history_index].result);
    } else {
        snprintf(history_line, sizeof history_line, "NO HISTORY");
        snprintf(result_line, sizeof result_line, "RESULT %s", result_text);
    }

    lcd_fill_rect(0, 0, LCD_WIDTH, DISPLAY_H, COL_BG);
    lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 18, expression_editor_view(editor, 38),
                  COL_TEXT, COL_BG, 2);
    lcd_draw_text(6, 47, tail_for_display(history_line, 78),
                  COL_TEXT, COL_BG, 1);
    lcd_draw_text(6, 64, tail_for_display(result_line, 78),
                  COL_MUTED, COL_BG, 1);
    lcd_fill_rect(0, DISPLAY_H - 2, LCD_WIDTH, 2, COL_MUTED);
}

static void render_display(void) {
    if (page == PAGE_PROGRAMMER) {
        render_programmer_display();
        return;
    }
    if (page == PAGE_FORMAT) {
        render_format_display();
        return;
    }
    if (page == PAGE_TOOLS) {
        render_tools_display();
        return;
    }
    if (page == PAGE_GRAPH) return;

    char status[48];
    snprintf(status, sizeof status, "%s  %s  %s",
             calc_engine_uses_degrees() ? "DEG" : "RAD",
             page == PAGE_SCIENTIFIC ? "SCIENTIFIC" : "BASIC", message);

    lcd_fill_rect(0, 0, LCD_WIDTH, DISPLAY_H, COL_BG);
    lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
    char editor[EXPR_MAX + 2];
    lcd_draw_text(6, 20, expression_editor_view(editor, 38),
                  COL_TEXT, COL_BG, 2);

    char shown_result[36];
    snprintf(shown_result, sizeof shown_result, "=%s", result_text);
    const char *visible = tail_for_display(shown_result, 38);
    int width = (int)strlen(visible) * 12;
    int x = LCD_WIDTH - width - 6;
    if (x < 6) x = 6;
    lcd_draw_text(x, 52, visible, COL_TEXT, COL_BG, 2);
    lcd_fill_rect(0, DISPLAY_H - 2, LCD_WIDTH, 2, COL_MUTED);
}

static void render_keypad(void) {
    size_t count;
    const calc_key_t *keys = active_keys(&count);
    lcd_fill_rect(0, DISPLAY_H, LCD_WIDTH, LCD_HEIGHT - DISPLAY_H, COL_BG);
    for (size_t i = 0; i < count; ++i) draw_key(&keys[i], false);
}

static double graph_grid_step(double span, int pixels, int target_pixels) {
    double raw_step = span * target_pixels / pixels;
    if (!(raw_step > 0.0) || !isfinite(raw_step)) return 1.0;

    double magnitude = pow(10.0, floor(log10(raw_step)));
    if (!(magnitude > 0.0) || !isfinite(magnitude)) return raw_step;

    double normalized = raw_step / magnitude;
    double multiple = normalized < 1.5 ? 1.0 :
                      normalized < 3.5 ? 2.0 :
                      normalized < 7.5 ? 5.0 : 10.0;
    return multiple * magnitude;
}

static int graph_x_pixel(double value, double x_min, double x_span) {
    return (int)(((value - x_min) * (LCD_WIDTH - 1) / x_span) + 0.5);
}

static int graph_y_pixel(double value, double y_max, double y_span,
                         int plot_top, int plot_height) {
    return plot_top +
           (int)(((y_max - value) * (plot_height - 1) / y_span) + 0.5);
}

static void format_graph_tick(char *buffer, size_t size, double value,
                              double step) {
    if (fabs(value) < step * 0.0001) value = 0.0;

    double absolute = fabs(value);
    if ((absolute > 0.0 && absolute < 0.001) || absolute >= 100000.0 ||
        step < 0.001 || step >= 100000.0) {
        snprintf(buffer, size, "%.2g", value);
        return;
    }

    int decimals = step < 1.0 ? (int)ceil(-log10(step)) : 0;
    if (decimals < 0) decimals = 0;
    if (decimals > 4) decimals = 4;
    snprintf(buffer, size, "%.*f", decimals, value);
}

static void draw_graph_grid(double x_min, double x_span,
                            double y_min, double y_span,
                            int plot_top, int plot_bottom,
                            double x_step, double y_step) {
    int plot_height = plot_bottom - plot_top;
    double x_max = x_min + x_span;
    double y_max = y_min + y_span;
    double first_x = ceil(x_min / x_step) * x_step;
    double first_y = ceil(y_min / y_step) * y_step;

    for (int tick = 0; tick < 64; ++tick) {
        double value = first_x + tick * x_step;
        if (value > x_max + x_step * 0.001) break;
        int pixel_x = graph_x_pixel(value, x_min, x_span);
        lcd_fill_rect(pixel_x, plot_top, 1, plot_height, COL_GRID);
    }
    for (int tick = 0; tick < 64; ++tick) {
        double value = first_y + tick * y_step;
        if (value > y_max + y_step * 0.001) break;
        int pixel_y = graph_y_pixel(value, y_max, y_span,
                                    plot_top, plot_height);
        lcd_fill_rect(0, pixel_y, LCD_WIDTH, 1, COL_GRID);
    }

    if (x_min <= 0.0 && x_max >= 0.0) {
        int axis_x = graph_x_pixel(0.0, x_min, x_span);
        lcd_fill_rect(axis_x, plot_top, 1, plot_height, COL_MUTED);
    }
    if (y_min <= 0.0 && y_max >= 0.0) {
        int axis_y = graph_y_pixel(0.0, y_max, y_span,
                                   plot_top, plot_height);
        lcd_fill_rect(0, axis_y, LCD_WIDTH, 1, COL_MUTED);
    }
}

static void draw_graph_axis_labels(double x_min, double x_span,
                                   double y_min, double y_span,
                                   int plot_top, int plot_bottom,
                                   double x_step, double y_step) {
    int plot_height = plot_bottom - plot_top;
    double x_max = x_min + x_span;
    double y_max = y_min + y_span;
    bool x_axis_visible = y_min <= 0.0 && y_max >= 0.0;
    bool y_axis_visible = x_min <= 0.0 && x_max >= 0.0;
    int axis_y = x_axis_visible
        ? graph_y_pixel(0.0, y_max, y_span, plot_top, plot_height)
        : plot_bottom - 1;
    int axis_x = y_axis_visible
        ? graph_x_pixel(0.0, x_min, x_span) : 0;
    int x_label_y = x_axis_visible ? axis_y + 3 : plot_bottom - 9;

    if (x_label_y + 8 > plot_bottom) x_label_y = axis_y - 10;
    if (x_label_y < plot_top) x_label_y = plot_top;

    double first_x = ceil(x_min / x_step) * x_step;
    for (int tick = 0; tick < 64; ++tick) {
        double value = first_x + tick * x_step;
        if (value > x_max + x_step * 0.001) break;

        char label[16];
        format_graph_tick(label, sizeof label, value, x_step);
        int label_width = (int)strlen(label) * 6;
        int label_x = graph_x_pixel(value, x_min, x_span) - label_width / 2;
        if (label_x < 1) label_x = 1;
        if (label_x + label_width >= LCD_WIDTH) {
            label_x = LCD_WIDTH - label_width - 1;
        }
        lcd_draw_text(label_x, x_label_y, label, COL_MUTED, COL_BG, 1);
    }

    double first_y = ceil(y_min / y_step) * y_step;
    for (int tick = 0; tick < 64; ++tick) {
        double value = first_y + tick * y_step;
        if (value > y_max + y_step * 0.001) break;
        if (x_axis_visible && fabs(value) < y_step * 0.0001) continue;

        char label[16];
        format_graph_tick(label, sizeof label, value, y_step);
        int label_width = (int)strlen(label) * 6;
        int label_x = 2;
        if (y_axis_visible) {
            label_x = axis_x + 4;
            if (label_x + label_width >= LCD_WIDTH) {
                label_x = axis_x - label_width - 3;
            }
        }
        int label_y = graph_y_pixel(value, y_max, y_span,
                                    plot_top, plot_height) - 4;
        if (label_y < plot_top) label_y = plot_top;
        if (label_y + 8 > plot_bottom) label_y = plot_bottom - 8;
        lcd_draw_text(label_x, label_y, label, COL_MUTED, COL_BG, 1);
    }

    if (x_axis_visible) {
        int name_y = x_label_y > axis_y ? axis_y - 10 : axis_y + 3;
        if (name_y < plot_top) name_y = plot_top;
        if (name_y + 8 > plot_bottom) name_y = plot_bottom - 8;
        lcd_draw_text(LCD_WIDTH - 8, name_y, "X", COL_TEXT, COL_BG, 1);
    }
    if (y_axis_visible) {
        int name_x = axis_x >= 9 ? axis_x - 8 : axis_x + 4;
        lcd_draw_text(name_x, plot_top + 1, "Y", COL_TEXT, COL_BG, 1);
    }
}

static void render_graph(void) {
    const int plot_top = 18;
    const int plot_bottom = KEY_Y + 4 * (KEY_H + KEY_GAP_Y) - 4;
    const int plot_height = plot_bottom - plot_top;
    double x_min = graph_x_center - graph_x_span * 0.5;
    double y_min = graph_y_center - graph_y_span * 0.5;
    double y_max = graph_y_center + graph_y_span * 0.5;
    double x_step = graph_grid_step(graph_x_span, LCD_WIDTH, 56);
    double y_step = graph_grid_step(graph_y_span, plot_height, 44);
    int error_position = 0;

    lcd_fill(COL_BG);
    draw_graph_grid(x_min, graph_x_span, y_min, graph_y_span,
                    plot_top, plot_bottom, x_step, y_step);

    calc_compiled_t *compiled = calc_engine_compile_x(expression, ans,
                                                       &error_position);
    bool previous_valid = false;
    int previous_y = 0;
    if (compiled) {
        for (int pixel_x = 0; pixel_x < LCD_WIDTH; ++pixel_x) {
            double x = x_min + graph_x_span * pixel_x / (LCD_WIDTH - 1);
            double y = 0.0;
            bool valid = calc_engine_evaluate_x(compiled, x, &y) &&
                         y >= y_min && y <= y_max;
            if (valid) {
                int pixel_y = plot_top + (int)((y_max - y) * (plot_height - 1) /
                                                graph_y_span);
                if (previous_valid &&
                    (pixel_y > previous_y ? pixel_y - previous_y
                                          : previous_y - pixel_y) < plot_height / 2) {
                    int top = pixel_y < previous_y ? pixel_y : previous_y;
                    int height = pixel_y > previous_y
                        ? pixel_y - previous_y + 1 : previous_y - pixel_y + 1;
                    lcd_fill_rect(pixel_x, top, 1, height, COL_TEXT);
                } else {
                    lcd_fill_rect(pixel_x, pixel_y, 1, 1, COL_TEXT);
                }
                previous_y = pixel_y;
            }
            previous_valid = valid;
        }
        calc_engine_free(compiled);
        snprintf(message, sizeof message, "GRAPH READY");
    } else {
        snprintf(message, sizeof message, "GRAPH SYNTAX %d", error_position);
    }

    draw_graph_axis_labels(x_min, graph_x_span, y_min, graph_y_span,
                           plot_top, plot_bottom, x_step, y_step);

    char status[80];
    snprintf(status, sizeof status, "GRAPH X %.4g..%.4g  Y %.4g..%.4g",
             x_min, x_min + graph_x_span, y_min, y_max);
    lcd_fill_rect(0, 0, LCD_WIDTH, 16, COL_BG);
    lcd_draw_text(6, 4, status, COL_TEXT, COL_BG, 1);
    for (size_t i = 0; i < sizeof graph_keys / sizeof graph_keys[0]; ++i) {
        draw_key(&graph_keys[i], false);
    }
}

static bool token_is_operator(const char *token) {
    return token && token[0] && token[1] == '\0' && strchr("+-*/^%", token[0]);
}

static void insert_token(const char *token) {
    if (!token || !*token) return;

    if (just_evaluated) {
        if (token_is_operator(token)) {
            snprintf(expression, sizeof expression, "ans");
            expression_len = 3;
            expression_cursor = expression_len;
        } else {
            expression[0] = '\0';
            expression_len = 0;
            expression_cursor = 0;
        }
        just_evaluated = false;
    }

    size_t token_len = strlen(token);
    if (expression_len + token_len >= sizeof expression) {
        snprintf(message, sizeof message, "INPUT FULL");
        return;
    }

    memmove(expression + expression_cursor + token_len,
            expression + expression_cursor,
            expression_len - expression_cursor + 1);
    memcpy(expression + expression_cursor, token, token_len);
    expression_len += token_len;
    expression_cursor += token_len;
    snprintf(message, sizeof message, "READY");
}

static void delete_expression_char(void) {
    if (expression_cursor == 0 || expression_len == 0) return;
    memmove(expression + expression_cursor - 1,
            expression + expression_cursor,
            expression_len - expression_cursor + 1);
    expression_cursor--;
    expression_len--;
    just_evaluated = false;
}

static void add_history(double value) {
    if (!expression_len) return;
    if (history_count == HISTORY_MAX) {
        memmove(&history[0], &history[1],
                (HISTORY_MAX - 1) * sizeof history[0]);
        history_count--;
    }
    snprintf(history[history_count].expression,
             sizeof history[history_count].expression, "%s", expression);
    snprintf(history[history_count].result,
             sizeof history[history_count].result, "%s", result_text);
    history[history_count].value = value;
    history_count++;
    history_index = history_count - 1;
}

static void evaluate_expression(void) {
    double value = 0.0;
    int error_position = 0;
    calc_status_t status = calc_engine_evaluate(expression, ans, &value,
                                                 &error_position);
    if (status == CALC_OK) {
        ans = value;
        snprintf(result_text, sizeof result_text, "%.12g", value);
        add_history(value);
        snprintf(message, sizeof message, "OK");
        just_evaluated = true;
        board_beep(1800, 18);
    } else {
        if (status == CALC_PARSE_ERROR && error_position > 0) {
            snprintf(message, sizeof message, "SYNTAX AT %d", error_position);
        } else {
            snprintf(message, sizeof message, "%s", calc_engine_status_text(status));
        }
        board_beep(500, 60);
    }
}

static void activate_programmer_key(const calc_key_t *key) {
    switch (key->action) {
        case ACT_PROG_DIGIT: {
            programmer_input_status_t status =
                programmer_engine_append(&programmer, key->token[0]);
            if (status == PROGRAMMER_INPUT_INVALID_DIGIT) {
                snprintf(message, sizeof message, "INVALID IN %s",
                         programmer_engine_base_name(programmer.base));
            } else if (status == PROGRAMMER_INPUT_OVERFLOW) {
                snprintf(message, sizeof message, "64-BIT OVERFLOW");
            } else {
                snprintf(message, sizeof message, "READY");
            }
            break;
        }
        case ACT_PROG_BASE:
            if (strcmp(key->token, "BIN") == 0) {
                programmer_engine_set_base(&programmer, PROGRAMMER_BIN);
            } else if (strcmp(key->token, "HEX") == 0) {
                programmer_engine_set_base(&programmer, PROGRAMMER_HEX);
            } else {
                programmer_engine_set_base(&programmer, PROGRAMMER_DEC);
            }
            snprintf(message, sizeof message, "%s MODE",
                     programmer_engine_base_name(programmer.base));
            render_keypad();
            break;
        case ACT_PROG_BINARY:
            if (strcmp(key->token, "AND") == 0) {
                programmer_engine_set_binary_op(&programmer, PROGRAMMER_OP_AND);
            } else if (strcmp(key->token, "OR") == 0) {
                programmer_engine_set_binary_op(&programmer, PROGRAMMER_OP_OR);
            } else {
                programmer_engine_set_binary_op(&programmer, PROGRAMMER_OP_XOR);
            }
            snprintf(message, sizeof message, "ENTER RIGHT VALUE");
            break;
        case ACT_PROG_UNARY:
            if (strcmp(key->token, "SHL") == 0) {
                programmer_engine_shift_left(&programmer);
            } else if (strcmp(key->token, "SHR") == 0) {
                programmer_engine_shift_right(&programmer);
            } else if (strcmp(key->token, "NOT") == 0) {
                programmer_engine_not(&programmer);
            } else {
                programmer_engine_negate(&programmer);
            }
            snprintf(message, sizeof message, "OK");
            break;
        default:
            break;
    }
}

static void set_programmer_value(uint64_t value) {
    programmer.value = value;
    programmer.replace_input = true;
    programmer_engine_set_base(&programmer, programmer.base);
}

static void activate_format_key(const calc_key_t *key) {
    uint64_t mask = number_format_mask(format_bits);

    if (key->action == ACT_FMT_WIDTH) {
        format_bits = key->token[0] == '8' ? 8u :
            (key->token[0] == '1' ? 16u : (key->token[0] == '3' ? 32u : 64u));
        if (fixed_fraction_bits >= format_bits) fixed_fraction_bits = format_bits - 1u;
        snprintf(message, sizeof message, "%u-BIT WORD", format_bits);
        render_keypad();
        return;
    }

    if (key->action == ACT_FMT_GOTO_BASE) {
        programmer_base_t base = strcmp(key->token, "BIN") == 0
            ? PROGRAMMER_BIN
            : (strcmp(key->token, "HEX") == 0 ? PROGRAMMER_HEX : PROGRAMMER_DEC);
        programmer_engine_set_base(&programmer, base);
        page = PAGE_PROGRAMMER;
        snprintf(message, sizeof message, "%s MODE",
                 programmer_engine_base_name(base));
        render_keypad();
        return;
    }

    if (strcmp(key->token, "NEG") == 0) {
        set_programmer_value(number_format_twos_negate(programmer.value,
                                                       format_bits));
        snprintf(message, sizeof message, "TWOS COMPLEMENT");
    } else if (strcmp(key->token, "MASK") == 0) {
        set_programmer_value(programmer.value & mask);
        snprintf(message, sizeof message, "MASK APPLIED");
    } else if (strcmp(key->token, "SEXT") == 0) {
        set_programmer_value(number_format_sign_extend(programmer.value,
                                                       format_bits));
        format_bits = 64;
        snprintf(message, sizeof message, "SIGN EXTENDED");
        render_keypad();
    } else if (strcmp(key->token, "Q-") == 0) {
        if (fixed_fraction_bits > 0) fixed_fraction_bits--;
        snprintf(message, sizeof message, "Q FRACTION -1");
    } else if (strcmp(key->token, "Q+") == 0) {
        if (fixed_fraction_bits + 1u < format_bits) fixed_fraction_bits++;
        snprintf(message, sizeof message, "Q FRACTION +1");
    } else if (strcmp(key->token, "Q0") == 0) {
        fixed_fraction_bits = 0;
        snprintf(message, sizeof message, "Q0 FORMAT");
    } else if (strcmp(key->token, "Q8") == 0 ||
               strcmp(key->token, "Q16") == 0 ||
               strcmp(key->token, "Q24") == 0) {
        unsigned int requested = key->token[1] == '8' ? 8u :
            (key->token[1] == '1' ? 16u : 24u);
        fixed_fraction_bits = requested < format_bits ? requested : format_bits - 1u;
        snprintf(message, sizeof message, "Q%u FORMAT", fixed_fraction_bits);
    } else if (strcmp(key->token, "A32") == 0) {
        set_programmer_value(number_format_float32_bits(ans));
        format_bits = 32;
        programmer_engine_set_base(&programmer, PROGRAMMER_HEX);
        snprintf(message, sizeof message, "ANS TO FLOAT32");
        render_keypad();
    } else if (strcmp(key->token, "A64") == 0) {
        set_programmer_value(number_format_float64_bits(ans));
        format_bits = 64;
        programmer_engine_set_base(&programmer, PROGRAMMER_HEX);
        snprintf(message, sizeof message, "ANS TO FLOAT64");
        render_keypad();
    } else if (strcmp(key->token, "32A") == 0) {
        ans = number_format_bits_float32((uint32_t)programmer.value);
        snprintf(result_text, sizeof result_text, "%.12g", ans);
        snprintf(message, sizeof message, "FLOAT32 TO ANS");
    } else if (strcmp(key->token, "64A") == 0) {
        ans = number_format_bits_float64(programmer.value);
        snprintf(result_text, sizeof result_text, "%.12g", ans);
        snprintf(message, sizeof message, "FLOAT64 TO ANS");
    } else if (strcmp(key->token, "ZERO") == 0) {
        set_programmer_value(0);
        snprintf(message, sizeof message, "ZERO");
    } else if (strcmp(key->token, "ONES") == 0) {
        set_programmer_value(mask);
        snprintf(message, sizeof message, "ALL ONES");
    } else if (strcmp(key->token, "MIN") == 0) {
        set_programmer_value(UINT64_C(1) << (format_bits - 1u));
        snprintf(message, sizeof message, "SIGNED MIN");
    } else if (strcmp(key->token, "MAX") == 0) {
        set_programmer_value((UINT64_C(1) << (format_bits - 1u)) - 1u);
        snprintf(message, sizeof message, "SIGNED MAX");
    } else if (strcmp(key->token, "INC") == 0) {
        set_programmer_value((programmer.value + 1u) & mask);
        snprintf(message, sizeof message, "+1");
    } else if (strcmp(key->token, "DEC") == 0) {
        set_programmer_value((programmer.value - 1u) & mask);
        snprintf(message, sizeof message, "-1");
    } else if (strcmp(key->token, "SHL") == 0) {
        set_programmer_value((programmer.value << 1u) & mask);
        snprintf(message, sizeof message, "SHIFT LEFT");
    } else if (strcmp(key->token, "SHR") == 0) {
        set_programmer_value((programmer.value & mask) >> 1u);
        snprintf(message, sizeof message, "SHIFT RIGHT");
    }
}

static void activate_memory_key(const calc_key_t *key) {
    if (strcmp(key->token, "M+") == 0) {
        memory_value += ans;
        snprintf(message, sizeof message, "MEMORY PLUS");
    } else if (strcmp(key->token, "M-") == 0) {
        memory_value -= ans;
        snprintf(message, sizeof message, "MEMORY MINUS");
    } else if (strcmp(key->token, "MR") == 0) {
        char token[32];
        snprintf(token, sizeof token, "%.12g", memory_value);
        insert_token(token);
        snprintf(message, sizeof message, "MEMORY RECALLED");
    } else {
        memory_value = 0.0;
        snprintf(message, sizeof message, "MEMORY CLEARED");
    }
}

static void activate_history_key(const calc_key_t *key) {
    if (strcmp(key->token, "CLEAR") == 0) {
        history_count = 0;
        history_index = 0;
        snprintf(message, sizeof message, "HISTORY CLEARED");
        return;
    }
    if (!history_count) {
        snprintf(message, sizeof message, "NO HISTORY");
        return;
    }
    if (strcmp(key->token, "PREV") == 0) {
        if (history_index > 0) history_index--;
        snprintf(message, sizeof message, "OLDER RESULT");
    } else if (strcmp(key->token, "NEXT") == 0) {
        if (history_index + 1 < history_count) history_index++;
        snprintf(message, sizeof message, "NEWER RESULT");
    } else {
        snprintf(expression, sizeof expression, "%s",
                 history[history_index].expression);
        expression_len = strlen(expression);
        expression_cursor = expression_len;
        snprintf(result_text, sizeof result_text, "%s",
                 history[history_index].result);
        ans = history[history_index].value;
        just_evaluated = false;
        page = PAGE_BASIC;
        snprintf(message, sizeof message, "HISTORY LOADED");
        render_keypad();
    }
}

static void activate_cursor_key(const calc_key_t *key) {
    if (strcmp(key->token, "LEFT") == 0) {
        if (expression_cursor > 0) expression_cursor--;
    } else if (strcmp(key->token, "RIGHT") == 0) {
        if (expression_cursor < expression_len) expression_cursor++;
    } else if (strcmp(key->token, "HOME") == 0) {
        expression_cursor = 0;
    } else {
        expression_cursor = expression_len;
    }
    snprintf(message, sizeof message, "CURSOR %u",
             (unsigned int)expression_cursor);
}

static void activate_graph_key(const calc_key_t *key) {
    if (strcmp(key->token, "LEFT") == 0) {
        graph_x_center -= graph_x_span * 0.1;
    } else if (strcmp(key->token, "RIGHT") == 0) {
        graph_x_center += graph_x_span * 0.1;
    } else if (strcmp(key->token, "IN") == 0) {
        graph_x_span *= 0.5;
        graph_y_span *= 0.5;
    } else if (strcmp(key->token, "OUT") == 0) {
        graph_x_span *= 2.0;
        graph_y_span *= 2.0;
    }
    render_graph();
}

static void activate_key(const calc_key_t *key) {
    switch (key->action) {
        case ACT_INSERT:
            insert_token(key->token);
            break;
        case ACT_EVALUATE:
            if (page == PAGE_PROGRAMMER) {
                bool calculated = programmer_engine_equals(&programmer);
                snprintf(message, sizeof message, "%s",
                         calculated ? "OK" : "NO OPERATION");
                board_beep(calculated ? 1800 : 500, calculated ? 18 : 45);
            } else {
                evaluate_expression();
            }
            break;
        case ACT_CLEAR:
            if (page == PAGE_PROGRAMMER) {
                programmer_engine_clear(&programmer);
            } else {
                expression[0] = '\0';
                expression_len = 0;
                expression_cursor = 0;
                snprintf(result_text, sizeof result_text, "0");
                just_evaluated = false;
            }
            snprintf(message, sizeof message, "CLEARED");
            break;
        case ACT_DELETE:
            if (page == PAGE_PROGRAMMER) {
                programmer_engine_delete(&programmer);
            } else if (page != PAGE_FORMAT) {
                delete_expression_char();
            }
            snprintf(message, sizeof message, "READY");
            just_evaluated = false;
            break;
        case ACT_PAGE:
            if (page == PAGE_BASIC) page = PAGE_SCIENTIFIC;
            else if (page == PAGE_SCIENTIFIC) page = PAGE_PROGRAMMER;
            else if (page == PAGE_PROGRAMMER) page = PAGE_FORMAT;
            else if (page == PAGE_FORMAT) page = PAGE_TOOLS;
            else page = PAGE_BASIC;
            snprintf(message, sizeof message, "%s",
                     page == PAGE_BASIC ? "BASIC KEYS" :
                     (page == PAGE_SCIENTIFIC ? "SCI FUNCTIONS" :
                     (page == PAGE_PROGRAMMER ? "64-BIT MODE" :
                     (page == PAGE_FORMAT ? "NUMBER FORMATS" : "TOOLS"))));
            render_keypad();
            break;
        case ACT_ANGLE:
            calc_engine_set_degrees(!calc_engine_uses_degrees());
            snprintf(message, sizeof message, "%s MODE",
                     calc_engine_uses_degrees() ? "DEG" : "RAD");
            render_keypad();
            break;
        case ACT_PROG_DIGIT:
        case ACT_PROG_BASE:
        case ACT_PROG_BINARY:
        case ACT_PROG_UNARY:
            activate_programmer_key(key);
            break;
        case ACT_GOTO_PROGRAMMER:
            page = PAGE_PROGRAMMER;
            snprintf(message, sizeof message, "64-BIT MODE");
            render_keypad();
            break;
        case ACT_FMT_WIDTH:
        case ACT_FMT_ACTION:
        case ACT_FMT_GOTO_BASE:
            activate_format_key(key);
            break;
        case ACT_MEMORY:
            activate_memory_key(key);
            break;
        case ACT_HISTORY:
            activate_history_key(key);
            break;
        case ACT_CURSOR:
            activate_cursor_key(key);
            break;
        case ACT_GOTO_GRAPH:
            page = PAGE_GRAPH;
            render_graph();
            break;
        case ACT_GOTO_TOOLS:
            page = PAGE_TOOLS;
            snprintf(message, sizeof message, "TOOLS");
            render_keypad();
            break;
        case ACT_GRAPH:
            activate_graph_key(key);
            break;
    }
    render_display();
}

static const calc_key_t *hit_key(uint16_t x, uint16_t y) {
    size_t count;
    const calc_key_t *keys = active_keys(&count);
    for (size_t i = 0; i < count; ++i) {
        int bx = key_x(&keys[i]);
        int by = key_y(&keys[i]);
        if (x >= bx && x < bx + KEY_W && y >= by && y < by + KEY_H) {
            return &keys[i];
        }
    }
    return NULL;
}

void calculator_ui_init(void) {
    lcd_fill(COL_BG);
    calc_engine_set_degrees(true);
    programmer_engine_init(&programmer);
    snprintf(message, sizeof message, "%s",
             touch_is_ready() ? "READY" : "TOUCH ERROR");
    render_display();
    render_keypad();
}

void calculator_ui_task(void) {
    touch_point_t point;
    if (touch_read(&point) && point.updated) {
        if (!point.touched) {
            touch_locked = false;
        } else if (!touch_locked) {
            touch_locked = true;
            const calc_key_t *key = hit_key(point.x, point.y);
            if (key) {
                draw_key(key, true);
                sleep_ms(25);
                draw_key(key, false);
                activate_key(key);
                if (key->action != ACT_EVALUATE) board_beep(1500, 10);
            }
        }
    }

    static bool old_button1;
    static bool old_button2;
    static bool joystick_locked;
    bool button1 = board_button1_pressed();
    bool button2 = board_button2_pressed();
    if (button1 && !old_button1) {
        if (page == PAGE_PROGRAMMER) {
            bool calculated = programmer_engine_equals(&programmer);
            snprintf(message, sizeof message, "%s",
                     calculated ? "OK" : "NO OPERATION");
        } else if (page == PAGE_BASIC || page == PAGE_SCIENTIFIC ||
                   page == PAGE_TOOLS) {
            evaluate_expression();
        }
        render_display();
    }
    if (button2 && !old_button2) {
        if (page == PAGE_PROGRAMMER) {
            programmer_engine_delete(&programmer);
        } else if (page == PAGE_BASIC || page == PAGE_SCIENTIFIC ||
                   page == PAGE_TOOLS) {
            delete_expression_char();
        }
        render_display();
    }
    old_button1 = button1;
    old_button2 = button2;

    joystick_state_t joystick = board_read_joystick();
    bool joystick_active = joystick.left || joystick.right ||
                           joystick.up || joystick.down;
    if (!joystick_active) {
        joystick_locked = false;
    } else if (!joystick_locked) {
        joystick_locked = true;
        if (page == PAGE_GRAPH) {
            if (joystick.left) graph_x_center -= graph_x_span * 0.1;
            if (joystick.right) graph_x_center += graph_x_span * 0.1;
            if (joystick.up) graph_y_center += graph_y_span * 0.1;
            if (joystick.down) graph_y_center -= graph_y_span * 0.1;
            render_graph();
        } else if (page == PAGE_BASIC || page == PAGE_SCIENTIFIC ||
                   page == PAGE_TOOLS) {
            if (joystick.left && expression_cursor > 0) expression_cursor--;
            if (joystick.right && expression_cursor < expression_len) {
                expression_cursor++;
            }
            render_display();
        }
    }
}
