#include "calculator_widgets.h"

#include "calculator_keymaps.h"
#include "lcd_st7796.h"

#include <stdio.h>
#include <string.h>

#define COL_BG       RGB565(0, 0, 0)
#define COL_TEXT     RGB565(255, 255, 255)
#define COL_MUTED    RGB565(170, 170, 170)
#define COL_KEY      RGB565(238, 238, 238)
#define COL_FUNCTION RGB565(170, 170, 170)
#define COL_COMMAND  RGB565(60, 60, 60)
#define COL_GRAPH_F1 RGB565(0, 220, 255)
#define COL_GRAPH_F2 RGB565(255, 220, 0)
#define COL_GRAPH_F3 RGB565(255, 80, 190)

static bool data_focus;

void calculator_widget_set_data_focus(bool enabled) {
    data_focus = enabled;
}

bool calculator_widget_data_focus(void) {
    return data_focus;
}

int calculator_widget_display_height(void) {
    return data_focus ? CALCULATOR_DATA_FOCUS_DISPLAY_HEIGHT
                      : CALCULATOR_DISPLAY_HEIGHT;
}

int calculator_widget_key_top(unsigned int row) {
    int top = data_focus ? CALCULATOR_DATA_FOCUS_KEY_Y : CALCULATOR_KEY_Y;
    int height = data_focus ? CALCULATOR_DATA_FOCUS_KEY_HEIGHT
                            : CALCULATOR_KEY_HEIGHT;
    int gap = data_focus ? CALCULATOR_DATA_FOCUS_KEY_GAP_Y
                         : CALCULATOR_KEY_GAP_Y;
    return top + (int)row * (height + gap);
}

int calculator_widget_key_height(void) {
    return data_focus ? CALCULATOR_DATA_FOCUS_KEY_HEIGHT
                      : CALCULATOR_KEY_HEIGHT;
}

static int key_x(const calc_key_t *key) {
    return CALCULATOR_KEY_X +
           key->col * (CALCULATOR_KEY_WIDTH + CALCULATOR_KEY_GAP_X);
}

static int key_y(const calc_key_t *key) {
    return calculator_widget_key_top(key->row);
}

static const char *unit_category_token(unit_category_t category) {
    static const char *const tokens[] = {
        "LENGTH", "AREA", "VOLUME", "MASS", "TIME", "TEMPERATURE",
        "ANGLE", "PRESSURE", "ENERGY", "POWER"
    };
    return category >= UNIT_CATEGORY_LENGTH && category < UNIT_CATEGORY_COUNT
        ? tokens[category] : "";
}

static uint16_t key_fill(const calc_key_t *key, bool pressed,
                         const calculator_widget_state_t *state) {
    if (pressed) return COL_BG;
    if (state->page == PAGE_GRAPH && key->action == ACT_GRAPH &&
        key->token[0] == 'F' && key->token[1] >= '1' &&
        key->token[1] <= '3' && key->token[2] == '\0') {
        size_t index = (size_t)(key->token[1] - '1');
        if (state->graph_active_mask & (1u << index)) {
            const uint16_t colors[] = {COL_GRAPH_F1, COL_GRAPH_F2, COL_GRAPH_F3};
            return colors[index];
        }
        return COL_COMMAND;
    }
    if (state->page == PAGE_PROGRAMMER && key->action == ACT_PROG_BASE) {
        unsigned int key_base = strcmp(key->token, "BIN") == 0 ? 2u :
            (strcmp(key->token, "HEX") == 0 ? 16u : 10u);
        if (state->programmer_base == key_base) return COL_TEXT;
    }
    if (state->page == PAGE_FORMAT && key->action == ACT_FMT_WIDTH) {
        unsigned int key_bits = key->token[0] == '8' ? 8u :
            (key->token[0] == '1' ? 16u :
             (key->token[0] == '3' ? 32u : 64u));
        if (key_bits == state->format_bits) return COL_TEXT;
    }
    if (state->page == PAGE_FORMAT && key->action == ACT_FMT_VIEW) {
        bool selected = (strcmp(key->token, "CONV") == 0 &&
                         state->format_view == FORMAT_VIEW_CONVERSIONS) ||
            (strcmp(key->token, "BITS") == 0 &&
             state->format_view == FORMAT_VIEW_BITS) ||
            (strcmp(key->token, "IEEE32") == 0 &&
             state->format_view == FORMAT_VIEW_IEEE32) ||
            (strcmp(key->token, "IEEE64") == 0 &&
             state->format_view == FORMAT_VIEW_IEEE64);
        if (selected) return COL_TEXT;
    }
    if (state->page == PAGE_LOGIC && key->action == ACT_LOGIC &&
        state->logic_view == LOGIC_VIEW_GATES && key->token[0] >= 'A' &&
        key->token[0] <= 'F' && key->token[1] == '\0') {
        uint8_t bit = (uint8_t)(1u << (key->token[0] - 'A'));
        if (!(state->logic_variable_mask & bit)) return COL_COMMAND;
        if (state->logic_assignment & bit) return COL_TEXT;
    }
    if (state->page == PAGE_UNITS && key->action == ACT_UNITS) {
        bool selected_category =
            strcmp(key->token, unit_category_token(state->unit_category)) == 0;
        bool selected_view =
            (strcmp(key->token, "CONV") == 0 &&
             state->units_view == UNITS_VIEW_CONVERTER) ||
            (strcmp(key->token, "CONST") == 0 &&
             state->units_view == UNITS_VIEW_CONSTANTS);
        if (selected_category || selected_view) return COL_TEXT;
    }
    if (state->page == PAGE_STATISTICS && key->action == ACT_STATISTICS) {
        bool selected_mode =
            (strcmp(key->token, "1VAR") == 0 &&
             !state->statistics_two_variable) ||
            (strcmp(key->token, "2VAR") == 0 &&
             state->statistics_two_variable);
        bool selected_view =
            (strcmp(key->token, "DATA") == 0 &&
             state->statistics_view == STATISTICS_VIEW_DATA) ||
            (strcmp(key->token, "SUMMARY") == 0 &&
             state->statistics_view == STATISTICS_VIEW_SUMMARY) ||
            (strcmp(key->token, "REGRESSION") == 0 &&
             state->statistics_view == STATISTICS_VIEW_REGRESSION) ||
            (strcmp(key->token, "PLOT") == 0 &&
             state->statistics_view == STATISTICS_VIEW_PLOT);
        if (selected_mode || selected_view) return COL_TEXT;
    }
    switch (key->style) {
        case STYLE_NUMBER: return COL_KEY;
        case STYLE_FUNCTION: return COL_FUNCTION;
        case STYLE_COMMAND: return COL_COMMAND;
        case STYLE_EQUALS: return COL_TEXT;
        default: return COL_KEY;
    }
}

void calculator_widget_draw_key(const calc_key_t *key, bool pressed,
                                const calculator_widget_state_t *state) {
    int x = key_x(key);
    int y = key_y(key);
    char favorite_label[12];
    const char *label = key->action == ACT_ANGLE
        ? (state->degrees ? "DEG" : "RAD") : key->label;
    if (key->action == ACT_FAVORITE && key->token[0] >= '0' &&
        key->token[0] <= '5' && key->token[1] == '\0') {
        size_t index = (size_t)(key->token[0] - '0');
        if (state->favorites[index] && state->favorites[index][0]) {
            snprintf(favorite_label, sizeof favorite_label, "%.11s",
                     state->favorites[index]);
            label = favorite_label;
        }
    }
    if (key->action == ACT_FMT_ACTION &&
        strcmp(key->token, "SIGN") == 0) {
        label = state->programmer_signed ? "SIGNED" : "UNSIGNED";
    }
    if (state->page == PAGE_COMPLEX && key->action == ACT_COMPLEX &&
        strcmp(key->token, "VIEW") == 0) {
        label = state->complex_polar ? "POLAR" : "CART";
    }
    if (state->page == PAGE_STATISTICS && key->action == ACT_STATISTICS &&
        strcmp(key->token, "XY") == 0) {
        label = state->statistics_active_y ? "Y" : "X";
    }
    uint16_t fill = key_fill(key, pressed, state);
    bool disabled = false;
    if (state->page == PAGE_PROGRAMMER && key->action == ACT_PROG_DIGIT) {
        int digit = key->token[0] <= '9'
            ? key->token[0] - '0' : key->token[0] - 'A' + 10;
        disabled = digit >= (int)state->programmer_base;
        if (disabled && !pressed) fill = COL_COMMAND;
    }
    bool dark = fill == COL_BG || fill == COL_COMMAND;
    uint16_t foreground = disabled ? COL_MUTED : (dark ? COL_TEXT : COL_BG);
    bool selected_graph_function = state->page == PAGE_GRAPH &&
        key->action == ACT_GRAPH && key->token[0] == 'F' &&
        key->token[1] == (char)('1' + state->graph_selected_function) &&
        key->token[2] == '\0';
    uint16_t border = selected_graph_function ? COL_TEXT :
        (dark ? COL_TEXT : COL_BG);
    size_t label_length = strlen(label);
    int key_height = calculator_widget_key_height();
    int scale_x = (CALCULATOR_KEY_WIDTH - 8) / ((int)label_length * 6);
    int scale_y = (key_height - 8) / 8;
    uint8_t scale = (uint8_t)(scale_x < scale_y ? scale_x : scale_y);
    if (scale > 4) scale = 4;
    if (scale < 1) scale = 1;
    int text_width = (int)label_length * 6 * scale;
    int text_height = 8 * scale;

    lcd_fill_rect(x, y, CALCULATOR_KEY_WIDTH, key_height, fill);
    lcd_draw_rect(x, y, CALCULATOR_KEY_WIDTH, key_height, border);
    lcd_draw_rect(x + 1, y + 1, CALCULATOR_KEY_WIDTH - 2,
                  key_height - 2, border);
    lcd_draw_text(x + (CALCULATOR_KEY_WIDTH - text_width) / 2,
                  y + (key_height - text_height) / 2,
                  label, foreground, fill, scale);
}

void calculator_widget_render_keypad(calc_page_t page,
                                     const calculator_widget_state_t *state) {
    size_t count;
    const calc_key_t *keys = page == PAGE_GRAPH
        ? calculator_graph_keymap(state->graph_view, &count)
        : (page == PAGE_COMPLEX
           ? calculator_complex_keymap(state->complex_history, &count)
        : (page == PAGE_FORMAT
           ? calculator_format_keymap(state->format_view, &count)
           : calculator_keymap(page, &count)));
    int display_height = calculator_widget_display_height();
    lcd_fill_rect(0, display_height, LCD_WIDTH,
                  LCD_HEIGHT - display_height, COL_BG);
    for (size_t i = 0; i < count; ++i) {
        calculator_widget_draw_key(&keys[i], false, state);
    }
}

const calc_key_t *calculator_widget_hit_key(calc_page_t page,
                                            const calculator_widget_state_t *state,
                                            uint16_t x, uint16_t y) {
    size_t count;
    const calc_key_t *keys = page == PAGE_GRAPH
        ? calculator_graph_keymap(state->graph_view, &count)
        : (page == PAGE_COMPLEX
           ? calculator_complex_keymap(state->complex_history, &count)
        : (page == PAGE_FORMAT
           ? calculator_format_keymap(state->format_view, &count)
           : calculator_keymap(page, &count)));
    for (size_t i = 0; i < count; ++i) {
        int left = key_x(&keys[i]);
        int top = key_y(&keys[i]);
        int key_height = calculator_widget_key_height();
        if (x >= left && x < left + CALCULATOR_KEY_WIDTH &&
            y >= top && y < top + key_height) {
            return &keys[i];
        }
    }
    return NULL;
}

const char *calculator_widget_tail(const char *text, size_t max_chars) {
    size_t length = strlen(text);
    return length > max_chars ? text + length - max_chars : text;
}
