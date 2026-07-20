#include "calculator_widgets.h"

#include "calculator_keymaps.h"
#include "calculator_ui_theme.h"
#include "lcd_st7796.h"

#include <stdio.h>
#include <string.h>

#define COL_BG       UI_COLOR_BACKGROUND
#define COL_TEXT     UI_COLOR_TEXT
#define COL_MUTED    UI_COLOR_MUTED
#define COL_KEY      UI_COLOR_KEY
#define COL_FUNCTION UI_COLOR_FUNCTION
#define COL_COMMAND  UI_COLOR_SURFACE
#define COL_GRAPH_F1 RGB565(0, 220, 255)
#define COL_GRAPH_F2 RGB565(255, 220, 0)
#define COL_GRAPH_F3 RGB565(255, 80, 190)

#define PORTRAIT_DISPLAY_HEIGHT 126
#define PORTRAIT_DATA_FOCUS_DISPLAY_HEIGHT 252
#define PORTRAIT_KEY_Y 132
#define PORTRAIT_DATA_FOCUS_KEY_Y 258
#define PORTRAIT_KEY_HEIGHT 64
#define PORTRAIT_DATA_FOCUS_KEY_HEIGHT 39
#define PORTRAIT_KEY_GAP_Y 4
#define PORTRAIT_DATA_FOCUS_KEY_GAP_Y 3

static calculator_layout_t layout;
static calc_page_t widget_page = PAGE_BASIC;

static bool page_is_calculator(calc_page_t page) {
    return page == PAGE_BASIC || page == PAGE_SCIENTIFIC;
}

static bool page_is_app_grid(calc_page_t page) {
    return page == PAGE_LAUNCHER || page == PAGE_SETTINGS;
}

static unsigned int page_columns(calc_page_t page) {
    if (page_is_app_grid(page)) return 3u;
    return page_is_calculator(page) ? 5u : 6u;
}

static unsigned int page_rows(calc_page_t page) {
    if (page == PAGE_LAUNCHER) return 5u;
    if (page == PAGE_SETTINGS) return 4u;
    return page_is_calculator(page) ? 6u : 5u;
}

void calculator_widget_set_page(calc_page_t page) {
    widget_page = page;
}

void calculator_widget_set_layout(calculator_layout_t next_layout) {
    layout = next_layout < CALCULATOR_LAYOUT_COUNT
        ? next_layout : CALCULATOR_LAYOUT_STANDARD;
}

calculator_layout_t calculator_widget_layout(void) {
    return layout;
}

calculator_layout_t calculator_widget_cycle_layout(void) {
    layout = (calculator_layout_t)((layout + 1) % CALCULATOR_LAYOUT_COUNT);
    return layout;
}

void calculator_widget_set_data_focus(bool enabled) {
    layout = enabled ? CALCULATOR_LAYOUT_DATA_FOCUS
                     : CALCULATOR_LAYOUT_STANDARD;
}

bool calculator_widget_data_focus(void) {
    return layout == CALCULATOR_LAYOUT_DATA_FOCUS;
}

bool calculator_widget_fullscreen(void) {
    return layout == CALCULATOR_LAYOUT_FULLSCREEN;
}

bool calculator_widget_keypad_visible(void) {
    return layout != CALCULATOR_LAYOUT_FULLSCREEN;
}

int calculator_widget_display_height(void) {
    if (layout == CALCULATOR_LAYOUT_FULLSCREEN) return lcd_height();
    if (page_is_app_grid(widget_page)) return 20;
    if (lcd_orientation() == LCD_ORIENTATION_PORTRAIT) {
        return layout == CALCULATOR_LAYOUT_DATA_FOCUS
            ? PORTRAIT_DATA_FOCUS_DISPLAY_HEIGHT : PORTRAIT_DISPLAY_HEIGHT;
    }
    return layout == CALCULATOR_LAYOUT_DATA_FOCUS
        ? CALCULATOR_DATA_FOCUS_DISPLAY_HEIGHT : CALCULATOR_DISPLAY_HEIGHT;
}

int calculator_widget_key_top(unsigned int row) {
    if (layout == CALCULATOR_LAYOUT_FULLSCREEN) return lcd_height();
    if (page_is_app_grid(widget_page)) {
        int top = 24;
        int rows = (int)page_rows(widget_page);
        int height = (lcd_height() - top - UI_MARGIN -
                      (rows - 1) * UI_GAP) / rows;
        return top + (int)row * (height + UI_GAP);
    }
    bool compact = layout == CALCULATOR_LAYOUT_DATA_FOCUS;
    if (compact) {
        unsigned int final_row = page_rows(widget_page) - 1u;
        if (widget_page == PAGE_GRAPH && row == 4u) row = 0u;
        else if (row == final_row) row = 1u;
    }
    bool portrait = lcd_orientation() == LCD_ORIENTATION_PORTRAIT;
    int top = portrait
        ? (compact ? PORTRAIT_DATA_FOCUS_KEY_Y : PORTRAIT_KEY_Y)
        : (compact ? CALCULATOR_DATA_FOCUS_KEY_Y : CALCULATOR_KEY_Y);
    int rows = compact ? 2 : (int)page_rows(widget_page);
    int gap = UI_GAP;
    int height = (lcd_height() - top - UI_MARGIN -
                  (rows - 1) * gap) / rows;
    return top + (int)row * (height + gap);
}

int calculator_widget_key_width(void) {
    int columns = (int)page_columns(widget_page);
    return (lcd_width() - 2 * CALCULATOR_KEY_X -
            (columns - 1) * CALCULATOR_KEY_GAP_X) / columns;
}

int calculator_widget_key_height(void) {
    if (layout == CALCULATOR_LAYOUT_FULLSCREEN) return 0;
    if (page_is_app_grid(widget_page)) {
        int rows = (int)page_rows(widget_page);
        return (lcd_height() - 24 - UI_MARGIN -
                (rows - 1) * UI_GAP) / rows;
    }
    int top = lcd_orientation() == LCD_ORIENTATION_PORTRAIT
        ? (layout == CALCULATOR_LAYOUT_DATA_FOCUS
            ? PORTRAIT_DATA_FOCUS_KEY_Y : PORTRAIT_KEY_Y)
        : (layout == CALCULATOR_LAYOUT_DATA_FOCUS
            ? CALCULATOR_DATA_FOCUS_KEY_Y : CALCULATOR_KEY_Y);
    int rows = layout == CALCULATOR_LAYOUT_DATA_FOCUS
        ? 2 : (int)page_rows(widget_page);
    return (lcd_height() - top - UI_MARGIN - (rows - 1) * UI_GAP) / rows;
}

static int key_x(const calc_key_t *key) {
    return CALCULATOR_KEY_X +
           key->col * (calculator_widget_key_width() + CALCULATOR_KEY_GAP_X);
}

static int key_y(const calc_key_t *key) {
    unsigned int row = key->row;
    if (layout == CALCULATOR_LAYOUT_DATA_FOCUS &&
        !page_is_app_grid(widget_page)) {
        unsigned int final_row = page_rows(widget_page) - 1u;
        if (widget_page == PAGE_GRAPH) row = 0u;
        else if (row == final_row) row = 1u;
    }
    return calculator_widget_key_top(row);
}

static bool key_is_visible(const calc_key_t *key) {
    if (layout != CALCULATOR_LAYOUT_DATA_FOCUS ||
        page_is_app_grid(widget_page)) return true;
    if (widget_page == PAGE_GRAPH) return key->row == 4u;
    unsigned int final_row = page_rows(widget_page) - 1u;
    return key->row == 0u || key->row == final_row;
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
        if (key->token[0] == 'U' && key->token[1] >= '0' &&
            key->token[1] <= '5') {
            size_t index = state->units_selector_offset +
                           (size_t)(key->token[1] - '0');
            bool selected = state->units_view == UNITS_VIEW_CONSTANTS
                ? index == state->units_constant_index
                : (state->units_selector == UNITS_SELECTOR_TO
                    ? index == state->units_to_index
                    : index == state->units_from_index);
            if (selected) return UI_COLOR_PRIMARY;
        }
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
    if (state->page == PAGE_SETTINGS && key->action == ACT_SETTINGS) {
        bool selected = (strcmp(key->token, "BEEP") == 0 &&
                         state->beep_enabled) ||
            (strcmp(key->token, "LAND") == 0 &&
             lcd_orientation() == LCD_ORIENTATION_LANDSCAPE) ||
            (strcmp(key->token, "PORT") == 0 &&
             lcd_orientation() == LCD_ORIENTATION_PORTRAIT) ||
            (strcmp(key->token, "STANDARD") == 0 &&
             state->default_layout == CALCULATOR_LAYOUT_STANDARD) ||
            (strcmp(key->token, "FOCUS") == 0 &&
             state->default_layout == CALCULATOR_LAYOUT_DATA_FOCUS) ||
            (strcmp(key->token, "FULL") == 0 &&
             state->default_layout == CALCULATOR_LAYOUT_FULLSCREEN);
        if (selected) return UI_COLOR_PRIMARY;
    }
    if (state->page == PAGE_NUMBER_THEORY &&
        key->action == ACT_NUMBER_THEORY && key->token[1] == '\0' &&
        key->token[0] >= 'A' && key->token[0] <= 'C' &&
        state->number_theory_input == (uint8_t)(key->token[0] - 'A')) {
        return UI_COLOR_PRIMARY;
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
    if (!calculator_widget_keypad_visible()) return;
    if (!key_is_visible(key)) return;
    int x = key_x(key);
    int y = key_y(key);
    char favorite_label[12];
    const char *label = key->action == ACT_ANGLE
        ? (state->degrees ? "DEG" : "RAD") : key->label;
    if (key->action == ACT_PRECISION) {
        label = calculator_precision_label(state->precision);
    }
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
    if (state->page == PAGE_UNITS && key->action == ACT_UNITS &&
        key->token[0] == 'U' && key->token[1] >= '0' &&
        key->token[1] <= '5') {
        size_t index = state->units_selector_offset +
                       (size_t)(key->token[1] - '0');
        if (state->units_view == UNITS_VIEW_CONSTANTS) {
            const physical_constant_t *constant =
                unit_engine_constant(index);
            label = constant ? constant->symbol : "--";
        } else {
            const unit_definition_t *unit =
                unit_engine_unit(state->unit_category, index);
            label = unit ? unit->symbol : "--";
        }
    }
    if (state->page == PAGE_STATISTICS && key->action == ACT_STATISTICS &&
        strcmp(key->token, "XY") == 0) {
        label = state->statistics_active_y ? "Y" : "X";
    }
    char fitted_label[16];
    int key_width = calculator_widget_key_width();
    size_t max_label_chars = key_width > 6
        ? (size_t)(key_width - 6) / 6u : 1u;
    if (strlen(label) > max_label_chars) {
        snprintf(fitted_label, sizeof fitted_label, "%.*s",
                 (int)max_label_chars, label);
        label = fitted_label;
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
    uint16_t foreground = disabled ? COL_MUTED :
        (dark ? COL_TEXT : UI_COLOR_TEXT_DARK);
    bool selected_graph_function = state->page == PAGE_GRAPH &&
        key->action == ACT_GRAPH && key->token[0] == 'F' &&
        key->token[1] == (char)('1' + state->graph_selected_function) &&
        key->token[2] == '\0';
    uint16_t border = selected_graph_function ? COL_TEXT :
        (dark ? COL_TEXT : COL_BG);
    size_t label_length = strlen(label);
    int key_height = calculator_widget_key_height();
    int scale_x = (key_width - 8) / ((int)label_length * 6);
    int scale_y = (key_height - 8) / 8;
    uint8_t scale = (uint8_t)(scale_x < scale_y ? scale_x : scale_y);
    if (scale > 4) scale = 4;
    if (scale < 1) scale = 1;
    int text_width = (int)label_length * 6 * scale;
    int text_height = 8 * scale;

    lcd_fill_rect(x, y, key_width, key_height, fill);
    lcd_draw_rect(x, y, key_width, key_height, border);
    lcd_draw_rect(x + 1, y + 1, key_width - 2,
                  key_height - 2, border);
    lcd_draw_text(x + (key_width - text_width) / 2,
                  y + (key_height - text_height) / 2,
                  label, foreground, fill, scale);
}

void calculator_widget_render_keypad(calc_page_t page,
                                     const calculator_widget_state_t *state) {
    calculator_widget_set_page(page);
    if (!calculator_widget_keypad_visible()) return;
    size_t count;
    const calc_key_t *keys = page == PAGE_UNITS
        ? calculator_units_keymap(state->units_view, state->units_selector,
                                  &count)
        : (page == PAGE_GRAPH
        ? calculator_graph_keymap(state->graph_view, &count)
        : (page == PAGE_COMPLEX
           ? calculator_complex_keymap(state->complex_history, &count)
        : (page == PAGE_FORMAT
           ? calculator_format_keymap(state->format_view, &count)
           : calculator_keymap(page, &count))));
    /* The graph renderer owns the complete frame up to its one-row toolbar.
     * Clearing from the generic 84-pixel display boundary would erase most
     * of the plot immediately after it was drawn. */
    if (page != PAGE_GRAPH) {
        int display_height = calculator_widget_display_height();
        lcd_fill_rect(0, display_height, lcd_width(),
                      lcd_height() - display_height, COL_BG);
    }
    for (size_t i = 0; i < count; ++i) {
        calculator_widget_draw_key(&keys[i], false, state);
    }
}

const calc_key_t *calculator_widget_hit_key(calc_page_t page,
                                            const calculator_widget_state_t *state,
                                            uint16_t x, uint16_t y) {
    calculator_widget_set_page(page);
    if (!calculator_widget_keypad_visible()) return NULL;
    size_t count;
    const calc_key_t *keys = page == PAGE_UNITS
        ? calculator_units_keymap(state->units_view, state->units_selector,
                                  &count)
        : (page == PAGE_GRAPH
        ? calculator_graph_keymap(state->graph_view, &count)
        : (page == PAGE_COMPLEX
           ? calculator_complex_keymap(state->complex_history, &count)
        : (page == PAGE_FORMAT
           ? calculator_format_keymap(state->format_view, &count)
           : calculator_keymap(page, &count))));
    for (size_t i = 0; i < count; ++i) {
        if (!key_is_visible(&keys[i])) continue;
        int left = key_x(&keys[i]);
        int top = key_y(&keys[i]);
        int key_width = calculator_widget_key_width();
        int key_height = calculator_widget_key_height();
        int touch_left = left - UI_GAP / 2;
        int touch_top = top - UI_GAP / 2;
        if (x >= touch_left && x < left + key_width + UI_GAP / 2 &&
            y >= touch_top && y < top + key_height + UI_GAP / 2) {
            return &keys[i];
        }
    }
    return NULL;
}

const char *calculator_widget_tail(const char *text, size_t max_chars) {
    size_t length = strlen(text);
    return length > max_chars ? text + length - max_chars : text;
}
