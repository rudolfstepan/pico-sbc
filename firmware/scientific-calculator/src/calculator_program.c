#include "calculator_program.h"

#include "lcd_st7796.h"
#include "pico/stdlib.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define COL_BG       RGB565(0, 0, 0)
#define COL_TEXT     RGB565(255, 255, 255)
#define COL_MUTED    RGB565(170, 170, 170)
#define COL_KEY      RGB565(238, 238, 238)
#define COL_FUNCTION RGB565(170, 170, 170)
#define COL_COMMAND  RGB565(60, 60, 60)

#define PROGRAM_KEYBOARD_Y 136
#define PROGRAM_COMPACT_KEYBOARD_Y 188
#define PROGRAM_PORTRAIT_KEYBOARD_Y 198
#define PROGRAM_PORTRAIT_COMPACT_KEYBOARD_Y 326
#define PROGRAM_KEYBOARD_HEIGHT \
    (LCD_LANDSCAPE_HEIGHT - PROGRAM_KEYBOARD_Y)

typedef enum {
    PROGRAM_KEY_INSERT,
    PROGRAM_KEY_DELETE,
    PROGRAM_KEY_ENTER,
    PROGRAM_KEY_NEW,
    PROGRAM_KEY_LAYER,
    PROGRAM_KEY_RUN,
    PROGRAM_KEY_EXIT
} program_key_action_t;

typedef struct {
    int x;
    int y;
    int w;
    int h;
    const char *label;
    const char *token;
    program_key_action_t action;
} program_key_t;

#define INSERT_KEY(x, y, w, h, label, token) \
    {x, y, w, h, label, token, PROGRAM_KEY_INSERT}

static const program_key_t alphabet_keys[] = {
    INSERT_KEY(8, 140, 42, 30, "1", "1"),
    INSERT_KEY(54, 140, 42, 30, "2", "2"),
    INSERT_KEY(100, 140, 42, 30, "3", "3"),
    INSERT_KEY(146, 140, 42, 30, "4", "4"),
    INSERT_KEY(192, 140, 42, 30, "5", "5"),
    INSERT_KEY(238, 140, 42, 30, "6", "6"),
    INSERT_KEY(284, 140, 42, 30, "7", "7"),
    INSERT_KEY(330, 140, 42, 30, "8", "8"),
    INSERT_KEY(376, 140, 42, 30, "9", "9"),
    INSERT_KEY(422, 140, 42, 30, "0", "0"),
    INSERT_KEY(8, 172, 42, 30, "Q", "Q"),
    INSERT_KEY(54, 172, 42, 30, "W", "W"),
    INSERT_KEY(100, 172, 42, 30, "E", "E"),
    INSERT_KEY(146, 172, 42, 30, "R", "R"),
    INSERT_KEY(192, 172, 42, 30, "T", "T"),
    INSERT_KEY(238, 172, 42, 30, "Z", "Z"),
    INSERT_KEY(284, 172, 42, 30, "U", "U"),
    INSERT_KEY(330, 172, 42, 30, "I", "I"),
    INSERT_KEY(376, 172, 42, 30, "O", "O"),
    INSERT_KEY(422, 172, 42, 30, "P", "P"),
    INSERT_KEY(20, 204, 38, 30, "A", "A"),
    INSERT_KEY(62, 204, 38, 30, "S", "S"),
    INSERT_KEY(104, 204, 38, 30, "D", "D"),
    INSERT_KEY(146, 204, 38, 30, "F", "F"),
    INSERT_KEY(188, 204, 38, 30, "G", "G"),
    INSERT_KEY(230, 204, 38, 30, "H", "H"),
    INSERT_KEY(272, 204, 38, 30, "J", "J"),
    INSERT_KEY(314, 204, 38, 30, "K", "K"),
    INSERT_KEY(356, 204, 38, 30, "L", "L"),
    {8, 236, 48, 30, "DEL", "", PROGRAM_KEY_DELETE},
    INSERT_KEY(60, 236, 44, 30, "Y", "Y"),
    INSERT_KEY(108, 236, 44, 30, "X", "X"),
    INSERT_KEY(156, 236, 44, 30, "C", "C"),
    INSERT_KEY(204, 236, 44, 30, "V", "V"),
    INSERT_KEY(252, 236, 44, 30, "B", "B"),
    INSERT_KEY(300, 236, 44, 30, "N", "N"),
    INSERT_KEY(348, 236, 44, 30, "M", "M"),
    {400, 204, 72, 62, "ENTER", "", PROGRAM_KEY_ENTER},
    {8, 268, 72, 28, "NEW", "", PROGRAM_KEY_NEW},
    {84, 268, 48, 28, "TOK", "", PROGRAM_KEY_LAYER},
    INSERT_KEY(136, 268, 164, 28, "SPACE", " "),
    {304, 268, 72, 28, "RUN", "", PROGRAM_KEY_RUN},
    {380, 268, 92, 28, "CALC", "", PROGRAM_KEY_EXIT},
};

static const program_key_t symbol_keys[] = {
    INSERT_KEY(8, 140, 42, 30, "+", "+"),
    INSERT_KEY(54, 140, 42, 30, "-", "-"),
    INSERT_KEY(100, 140, 42, 30, "*", "*"),
    INSERT_KEY(146, 140, 42, 30, "/", "/"),
    INSERT_KEY(192, 140, 42, 30, "^", "^"),
    INSERT_KEY(238, 140, 42, 30, "=", "="),
    INSERT_KEY(284, 140, 42, 30, "<", "<"),
    INSERT_KEY(330, 140, 42, 30, ">", ">"),
    INSERT_KEY(376, 140, 42, 30, "(", "("),
    INSERT_KEY(422, 140, 42, 30, ")", ")"),
    INSERT_KEY(8, 172, 42, 30, "LET", "LET "),
    INSERT_KEY(54, 172, 42, 30, "PRINT", "PRINT "),
    INSERT_KEY(100, 172, 42, 30, "INPUT", "INPUT "),
    INSERT_KEY(146, 172, 42, 30, "IF", "IF "),
    INSERT_KEY(192, 172, 42, 30, "THEN", "THEN "),
    INSERT_KEY(238, 172, 42, 30, "GOTO", "GOTO "),
    INSERT_KEY(284, 172, 42, 30, "FOR", "FOR "),
    INSERT_KEY(330, 172, 42, 30, "TO", " TO "),
    INSERT_KEY(376, 172, 42, 30, "STEP", " STEP "),
    INSERT_KEY(422, 172, 42, 30, "REM", "REM "),
    INSERT_KEY(20, 204, 38, 30, "NEXT", "NEXT "),
    INSERT_KEY(62, 204, 38, 30, "END", "END"),
    INSERT_KEY(104, 204, 38, 30, "CLS", "CLS"),
    INSERT_KEY(146, 204, 38, 30, "SIN", "SIN("),
    INSERT_KEY(188, 204, 38, 30, "COS", "COS("),
    INSERT_KEY(230, 204, 38, 30, "TAN", "TAN("),
    INSERT_KEY(272, 204, 38, 30, "SQRT", "SQRT("),
    INSERT_KEY(314, 204, 38, 30, "ABS", "ABS("),
    INSERT_KEY(356, 204, 38, 30, "EXP", "EXP("),
    {8, 236, 48, 30, "DEL", "", PROGRAM_KEY_DELETE},
    INSERT_KEY(60, 236, 44, 30, ".", "."),
    INSERT_KEY(108, 236, 44, 30, ",", ","),
    INSERT_KEY(156, 236, 44, 30, "QUOTE", "\""),
    INSERT_KEY(204, 236, 44, 30, "<=", "<="),
    INSERT_KEY(252, 236, 44, 30, ">=", ">="),
    INSERT_KEY(300, 236, 44, 30, "<>", "<>"),
    INSERT_KEY(348, 236, 44, 30, "PI", "PI"),
    {400, 204, 72, 62, "ENTER", "", PROGRAM_KEY_ENTER},
    {8, 268, 72, 28, "NEW", "", PROGRAM_KEY_NEW},
    {84, 268, 48, 28, "ABC", "", PROGRAM_KEY_LAYER},
    INSERT_KEY(136, 268, 164, 28, "SPACE", " "),
    {304, 268, 72, 28, "RUN", "", PROGRAM_KEY_RUN},
    {380, 268, 92, 28, "CALC", "", PROGRAM_KEY_EXIT},
};

static int program_keyboard_top(const calculator_program_t *program) {
    bool portrait = lcd_orientation() == LCD_ORIENTATION_PORTRAIT;
    switch (program->layout) {
        case CALCULATOR_LAYOUT_DATA_FOCUS:
            return portrait ? PROGRAM_PORTRAIT_COMPACT_KEYBOARD_Y
                            : PROGRAM_COMPACT_KEYBOARD_Y;
        case CALCULATOR_LAYOUT_FULLSCREEN:
            return lcd_height();
        case CALCULATOR_LAYOUT_STANDARD:
        default:
            return portrait ? PROGRAM_PORTRAIT_KEYBOARD_Y
                            : PROGRAM_KEYBOARD_Y;
    }
}

static size_t program_editor_visible_lines(
    const calculator_program_t *program) {
    if (lcd_orientation() == LCD_ORIENTATION_PORTRAIT) {
        return program->layout == CALCULATOR_LAYOUT_STANDARD ? 11u : 20u;
    }
    switch (program->layout) {
        case CALCULATOR_LAYOUT_DATA_FOCUS: return 10u;
        case CALCULATOR_LAYOUT_FULLSCREEN: return 19u;
        case CALCULATOR_LAYOUT_STANDARD:
        default: return 6u;
    }
}

static int program_prompt_top(const calculator_program_t *program) {
    if (lcd_orientation() == LCD_ORIENTATION_PORTRAIT) {
        switch (program->layout) {
            case CALCULATOR_LAYOUT_DATA_FOCUS: return 293;
            case CALCULATOR_LAYOUT_FULLSCREEN: return 446;
            case CALCULATOR_LAYOUT_STANDARD:
            default: return 165;
        }
    }
    switch (program->layout) {
        case CALCULATOR_LAYOUT_DATA_FOCUS: return 155;
        case CALCULATOR_LAYOUT_FULLSCREEN: return 286;
        case CALCULATOR_LAYOUT_STANDARD:
        default: return 103;
    }
}

static size_t program_output_visible_lines(
    const calculator_program_t *program) {
    bool input = program->engine.state == BASIC_RUN_INPUT;
    if (lcd_orientation() == LCD_ORIENTATION_PORTRAIT) {
        switch (program->layout) {
            case CALCULATOR_LAYOUT_DATA_FOCUS: return input ? 15u : 16u;
            case CALCULATOR_LAYOUT_FULLSCREEN: return input ? 15u : 16u;
            case CALCULATOR_LAYOUT_STANDARD:
            default: return input ? 8u : 10u;
        }
    }
    switch (program->layout) {
        case CALCULATOR_LAYOUT_DATA_FOCUS: return input ? 7u : 9u;
        case CALCULATOR_LAYOUT_FULLSCREEN: return input ? 15u : 16u;
        case CALCULATOR_LAYOUT_STANDARD:
        default: return input ? 5u : 6u;
    }
}

static program_key_t program_key_geometry(
    const calculator_program_t *program, const program_key_t *source) {
    program_key_t key = *source;
    int target_top = program_keyboard_top(program);
    int target_height = lcd_height() - target_top;
    if (lcd_orientation() == LCD_ORIENTATION_PORTRAIT) {
        key.x = source->x * lcd_width() / LCD_LANDSCAPE_WIDTH;
        key.w = source->w * lcd_width() / LCD_LANDSCAPE_WIDTH;
    }
    if (target_top != PROGRAM_KEYBOARD_Y ||
        target_height != PROGRAM_KEYBOARD_HEIGHT) {
        key.y = target_top +
            (source->y - PROGRAM_KEYBOARD_Y) *
            target_height / PROGRAM_KEYBOARD_HEIGHT;
        key.h = source->h * target_height / PROGRAM_KEYBOARD_HEIGHT;
        if (key.h < 16) key.h = 16;
    }
    return key;
}

static void keymap(const calculator_program_t *program,
                   const program_key_t **keys, size_t *count) {
    if (program->symbol_layer) {
        *keys = symbol_keys;
        *count = sizeof symbol_keys / sizeof symbol_keys[0];
    } else {
        *keys = alphabet_keys;
        *count = sizeof alphabet_keys / sizeof alphabet_keys[0];
    }
}

static bool program_is_active(const calculator_program_t *program) {
    return program->engine.state == BASIC_RUN_RUNNING ||
           program->engine.state == BASIC_RUN_INPUT;
}

static uint16_t key_fill(const program_key_t *key, bool pressed) {
    if (pressed) return COL_BG;
    if (key->action == PROGRAM_KEY_RUN ||
        key->action == PROGRAM_KEY_ENTER) {
        return COL_TEXT;
    }
    if (key->action == PROGRAM_KEY_INSERT) return COL_KEY;
    if (key->action == PROGRAM_KEY_LAYER) return COL_FUNCTION;
    return COL_COMMAND;
}

static void draw_key(const calculator_program_t *program,
                     const program_key_t *key, bool pressed) {
    if (program->layout == CALCULATOR_LAYOUT_FULLSCREEN) return;
    program_key_t geometry = program_key_geometry(program, key);
    const program_key_t *visible_key = &geometry;
    const char *label = visible_key->label;
    if (visible_key->action == PROGRAM_KEY_RUN && program_is_active(program)) {
        label = "STOP";
    }
    char fitted_label[16];
    size_t max_label_chars = visible_key->w > 6
        ? (size_t)(visible_key->w - 6) / 6u : 1u;
    if (strlen(label) > max_label_chars) {
        snprintf(fitted_label, sizeof fitted_label, "%.*s",
                 (int)max_label_chars, label);
        label = fitted_label;
    }
    uint16_t fill = key_fill(visible_key, pressed);
    bool dark = fill == COL_BG || fill == COL_COMMAND;
    uint16_t foreground = dark ? COL_TEXT : COL_BG;
    size_t length = strlen(label);
    int scale_x = (visible_key->w - 6) / ((int)length * 6);
    int scale_y = (visible_key->h - 6) / 8;
    uint8_t scale = (uint8_t)(scale_x < scale_y ? scale_x : scale_y);
    if (scale > 3) scale = 3;
    if (scale < 1) scale = 1;
    int text_width = (int)length * 6 * scale;
    int text_height = 8 * scale;
    lcd_fill_rect(visible_key->x, visible_key->y,
                  visible_key->w, visible_key->h, fill);
    lcd_draw_rect(visible_key->x, visible_key->y,
                  visible_key->w, visible_key->h,
                  dark ? COL_TEXT : COL_BG);
    lcd_draw_rect(visible_key->x + 1, visible_key->y + 1,
                  visible_key->w - 2, visible_key->h - 2,
                  dark ? COL_TEXT : COL_BG);
    lcd_draw_text(visible_key->x + (visible_key->w - text_width) / 2,
                  visible_key->y + (visible_key->h - text_height) / 2,
                  label, foreground, fill, scale);
}

static void render_keyboard(const calculator_program_t *program) {
    if (program->layout == CALCULATOR_LAYOUT_FULLSCREEN) return;
    const program_key_t *keys = NULL;
    size_t count = 0;
    keymap(program, &keys, &count);
    int top = program_keyboard_top(program);
    lcd_fill_rect(0, top, lcd_width(), lcd_height() - top, COL_BG);
    for (size_t i = 0; i < count; ++i) draw_key(program, &keys[i], false);
}

static const char *run_state_text(const calculator_program_t *program) {
    switch (program->engine.state) {
        case BASIC_RUN_RUNNING: return "RUN";
        case BASIC_RUN_INPUT: return "INPUT";
        case BASIC_RUN_FINISHED: return "DONE";
        case BASIC_RUN_ERROR: return "ERROR";
        case BASIC_RUN_STOPPED:
        default: return "EDIT";
    }
}

static void render_status(const calculator_program_t *program) {
    char status[80];
    snprintf(status, sizeof status, "CODE %u/%u  %s  %s  %s",
             (unsigned int)program->engine.program.count,
             BASIC_PROGRAM_MAX_LINES, run_state_text(program),
             program->symbol_layer ? "TOK" : "ABC", program->notice);
    size_t max_chars = (size_t)(lcd_width() - 8) / 6u;
    status[max_chars < sizeof status ? max_chars : sizeof status - 1u] = '\0';
    lcd_draw_text(4, 2, status, COL_MUTED, COL_BG, 1);
}

static void render_editor(calculator_program_t *program) {
    char line[BASIC_LINE_TEXT_CAPACITY + 8];
    size_t count = program->engine.program.count;
    size_t visible_lines = program_editor_visible_lines(program);
    size_t maximum_scroll = count > visible_lines
        ? count - visible_lines : 0;
    if (program->list_scroll > maximum_scroll) {
        program->list_scroll = maximum_scroll;
    }
    for (size_t row = 0; row < visible_lines; ++row) {
        size_t index = program->list_scroll + row;
        if (index >= count) break;
        snprintf(line, sizeof line, "%u %s",
                 (unsigned int)program->engine.program.lines[index].number,
                 program->engine.program.lines[index].text);
        size_t max_chars = (size_t)(lcd_width() - 12) / 6u;
        if (max_chars < sizeof line) line[max_chars] = '\0';
        lcd_draw_text(6, 16 + (int)row * 14, line, COL_TEXT, COL_BG, 1);
    }
    char input[EXPRESSION_EDITOR_CAPACITY + 2];
    char prompt[44];
    size_t prompt_chars = (size_t)(lcd_width() - 12) / 12u;
    size_t editor_chars = prompt_chars > 1u ? prompt_chars - 1u : 1u;
    snprintf(prompt, sizeof prompt, ">%.*s", (int)editor_chars,
             expression_editor_view(&program->editor, input, sizeof input,
                                    editor_chars));
    int prompt_top = program_prompt_top(program);
    lcd_fill_rect(0, prompt_top, lcd_width(),
                  lcd_height() - prompt_top, COL_BG);
    lcd_fill_rect(0, prompt_top, lcd_width(), 2, COL_MUTED);
    lcd_draw_text(6, prompt_top + 9, prompt, COL_TEXT, COL_BG, 2);
}

static size_t output_max_chars(void) {
    size_t max_chars = (size_t)(lcd_width() - 12) / 12u;
    if (max_chars > 39u) max_chars = 39u;
    return max_chars ? max_chars : 1u;
}

static size_t output_wrap_chunk(const char *source, size_t offset,
                                size_t max_chars, size_t *next_offset) {
    size_t length = strlen(source);
    size_t remaining = length - offset;
    size_t chunk = remaining < max_chars ? remaining : max_chars;
    if (remaining > max_chars) {
        size_t break_after = chunk;
        while (break_after > 0u &&
               source[offset + break_after - 1u] != ' ') {
            break_after--;
        }
        if (break_after > 1u) chunk = break_after - 1u;
    }
    size_t next = offset + chunk;
    while (next < length && source[next] == ' ') next++;
    *next_offset = next;
    return chunk;
}

static size_t output_wrapped_rows(const char *source, size_t max_chars) {
    if (!source || !*source) return 1u;
    size_t rows = 0;
    size_t offset = 0;
    size_t length = strlen(source);
    while (offset < length) {
        size_t next = offset;
        output_wrap_chunk(source, offset, max_chars, &next);
        rows++;
        if (next <= offset) break;
        offset = next;
    }
    return rows;
}

static bool output_wrapped_row(const char *source, size_t max_chars,
                               size_t target_row, char *destination,
                               size_t capacity) {
    if (!source || !destination || !capacity) return false;
    if (!*source) {
        destination[0] = '\0';
        return target_row == 0u;
    }
    size_t offset = 0;
    size_t length = strlen(source);
    size_t row = 0;
    while (offset < length) {
        size_t next = offset;
        size_t chunk = output_wrap_chunk(source, offset, max_chars, &next);
        if (row == target_row) {
            if (chunk >= capacity) chunk = capacity - 1u;
            memcpy(destination, source + offset, chunk);
            destination[chunk] = '\0';
            return true;
        }
        if (next <= offset) break;
        offset = next;
        row++;
    }
    return false;
}

static void render_output(const calculator_program_t *program) {
    size_t visible_lines = program_output_visible_lines(program);
    size_t max_chars = output_max_chars();
    size_t total_rows = 0;
    for (size_t i = 0; i < program->engine.output_count; ++i) {
        total_rows += output_wrapped_rows(program->engine.output[i], max_chars);
    }
    size_t first_visible = total_rows > visible_lines
        ? total_rows - visible_lines : 0;
    size_t visual_row = 0;
    size_t drawn_rows = 0;
    char output[40];
    for (size_t i = 0; i < program->engine.output_count &&
                       drawn_rows < visible_lines; ++i) {
        size_t rows = output_wrapped_rows(program->engine.output[i], max_chars);
        for (size_t row = 0; row < rows && drawn_rows < visible_lines; ++row) {
            if (visual_row++ < first_visible) continue;
            if (!output_wrapped_row(program->engine.output[i], max_chars, row,
                                    output, sizeof output)) {
                continue;
            }
            lcd_draw_text(6, 16 + (int)drawn_rows * 18, output,
                          COL_TEXT, COL_BG, 2);
            drawn_rows++;
        }
    }
    if (program->engine.state == BASIC_RUN_INPUT) {
        char input[EXPRESSION_EDITOR_CAPACITY + 2];
        char prompt[44];
        size_t prompt_chars = (size_t)(lcd_width() - 12) / 12u;
        size_t editor_chars = prompt_chars > 2u ? prompt_chars - 2u : 1u;
        snprintf(prompt, sizeof prompt, "? %.*s", (int)editor_chars,
                 expression_editor_view(&program->editor, input,
                                        sizeof input, editor_chars));
        int prompt_top = program_prompt_top(program);
        lcd_fill_rect(0, prompt_top, lcd_width(),
                      lcd_height() - prompt_top, COL_BG);
        lcd_fill_rect(0, prompt_top, lcd_width(), 2, COL_MUTED);
        lcd_draw_text(6, prompt_top + 9, prompt, COL_TEXT, COL_BG, 2);
    }
}

void calculator_program_init(calculator_program_t *program) {
    memset(program, 0, sizeof *program);
    basic_engine_init(&program->engine);
    expression_editor_init(&program->editor);
    snprintf(program->notice, sizeof program->notice, "READY");
}

void calculator_program_set_layout(calculator_program_t *program,
                                   calculator_layout_t layout) {
    program->layout = layout < CALCULATOR_LAYOUT_COUNT
        ? layout : CALCULATOR_LAYOUT_STANDARD;
    size_t visible_lines = program_editor_visible_lines(program);
    size_t count = program->engine.program.count;
    size_t maximum_scroll = count > visible_lines
        ? count - visible_lines : 0;
    if (program->list_scroll > maximum_scroll) {
        program->list_scroll = maximum_scroll;
    }
}

void calculator_program_set_source(calculator_program_t *program,
                                   const basic_program_t *source) {
    if (basic_engine_set_program(&program->engine, source) != BASIC_STATUS_OK) {
        basic_engine_clear_program(&program->engine);
    }
    program->list_scroll = 0;
    program->output_view = false;
    expression_editor_clear(&program->editor);
}

const basic_program_t *calculator_program_source(
    const calculator_program_t *program) {
    return &program->engine.program;
}

void calculator_program_render(calculator_program_t *program) {
    lcd_fill(COL_BG);
    render_status(program);
    if (program->output_view || program_is_active(program)) {
        render_output(program);
    } else {
        render_editor(program);
    }
    render_keyboard(program);
}

static unsigned int enter_input(calculator_program_t *program) {
    basic_status_t status;
    unsigned int effects = CALCULATOR_PROGRAM_RENDER |
                           CALCULATOR_PROGRAM_BEEP;
    if (program->engine.state == BASIC_RUN_INPUT) {
        status = basic_engine_submit_input(&program->engine,
                                           program->editor.text);
        if (status == BASIC_STATUS_OK) expression_editor_clear(&program->editor);
    } else {
        status = basic_engine_store_line(&program->engine,
                                         program->editor.text);
        if (status == BASIC_STATUS_OK) {
            expression_editor_clear(&program->editor);
            program->output_view = false;
            size_t count = program->engine.program.count;
            size_t visible_lines = program_editor_visible_lines(program);
            program->list_scroll = count > visible_lines
                ? count - visible_lines : 0;
            effects |= CALCULATOR_PROGRAM_DIRTY;
        }
    }
    snprintf(program->notice, sizeof program->notice, "%s",
             basic_status_text(status));
    return effects;
}

unsigned int calculator_program_enter(calculator_program_t *program) {
    if (program->engine.state == BASIC_RUN_RUNNING) {
        snprintf(program->notice, sizeof program->notice, "RUNNING");
        return CALCULATOR_PROGRAM_RENDER;
    }
    program->clear_armed = false;
    return enter_input(program);
}

static unsigned int activate_key(calculator_program_t *program,
                                 const program_key_t *key) {
    unsigned int effects = CALCULATOR_PROGRAM_RENDER |
                           CALCULATOR_PROGRAM_BEEP;
    bool clear_was_armed = program->clear_armed;
    if (key->action != PROGRAM_KEY_NEW) program->clear_armed = false;
    switch (key->action) {
        case PROGRAM_KEY_INSERT:
            if (program->engine.state == BASIC_RUN_RUNNING) {
                snprintf(program->notice, sizeof program->notice, "RUNNING");
                break;
            }
            if (program->engine.state != BASIC_RUN_INPUT) {
                program->output_view = false;
            }
            if (!expression_editor_insert(&program->editor, key->token)) {
                snprintf(program->notice, sizeof program->notice, "INPUT FULL");
            } else {
                snprintf(program->notice, sizeof program->notice, "READY");
            }
            break;
        case PROGRAM_KEY_DELETE:
            if (program->engine.state != BASIC_RUN_RUNNING) {
                if (program->engine.state != BASIC_RUN_INPUT) {
                    program->output_view = false;
                }
                expression_editor_delete(&program->editor);
            }
            break;
        case PROGRAM_KEY_ENTER:
            return calculator_program_enter(program);
        case PROGRAM_KEY_NEW:
            if (!clear_was_armed) {
                program->clear_armed = true;
                snprintf(program->notice, sizeof program->notice,
                         "NEW: TAP AGAIN");
            } else {
                basic_engine_clear_program(&program->engine);
                expression_editor_clear(&program->editor);
                program->list_scroll = 0;
                program->output_view = false;
                program->clear_armed = false;
                snprintf(program->notice, sizeof program->notice, "CLEARED");
                effects |= CALCULATOR_PROGRAM_DIRTY;
            }
            break;
        case PROGRAM_KEY_LAYER:
            program->symbol_layer = !program->symbol_layer;
            break;
        case PROGRAM_KEY_RUN: {
            if (program_is_active(program)) {
                basic_engine_stop(&program->engine);
                snprintf(program->notice, sizeof program->notice, "STOPPED");
            } else {
                basic_status_t status = basic_engine_run(&program->engine);
                snprintf(program->notice, sizeof program->notice, "%s",
                         basic_status_text(status));
            }
            program->output_view = true;
            expression_editor_clear(&program->editor);
            break;
        }
        case PROGRAM_KEY_EXIT:
            if (program_is_active(program)) basic_engine_stop(&program->engine);
            effects |= CALCULATOR_PROGRAM_EXIT;
            break;
    }
    return effects;
}

unsigned int calculator_program_touch(calculator_program_t *program,
                                      uint16_t x, uint16_t y) {
    if (y < program_keyboard_top(program)) {
        return calculator_program_toggle_view(program) |
               CALCULATOR_PROGRAM_BEEP;
    }
    const program_key_t *keys = NULL;
    size_t count = 0;
    keymap(program, &keys, &count);
    for (size_t i = 0; i < count; ++i) {
        const program_key_t *key = &keys[i];
        program_key_t geometry = program_key_geometry(program, key);
        if (x >= geometry.x && x < geometry.x + geometry.w &&
            y >= geometry.y && y < geometry.y + geometry.h) {
            draw_key(program, key, true);
            sleep_ms(25);
            return activate_key(program, key);
        }
    }
    return 0;
}

unsigned int calculator_program_toggle_view(calculator_program_t *program) {
    if (program_is_active(program)) return 0;
    program->output_view = !program->output_view;
    program->clear_armed = false;
    snprintf(program->notice, sizeof program->notice, "%s",
             program->output_view ? "OUTPUT" : "EDITOR");
    return CALCULATOR_PROGRAM_RENDER;
}

unsigned int calculator_program_move(calculator_program_t *program,
                                     int horizontal, int vertical) {
    if (horizontal < 0) {
        expression_editor_move(&program->editor, EDITOR_CURSOR_LEFT);
    } else if (horizontal > 0) {
        expression_editor_move(&program->editor, EDITOR_CURSOR_RIGHT);
    }
    if (!program->output_view && program->engine.state != BASIC_RUN_INPUT) {
        size_t count = program->engine.program.count;
        size_t visible_lines = program_editor_visible_lines(program);
        size_t maximum_scroll = count > visible_lines
            ? count - visible_lines : 0;
        if (vertical < 0 && program->list_scroll) program->list_scroll--;
        if (vertical > 0 && program->list_scroll < maximum_scroll) {
            program->list_scroll++;
        }
    }
    return CALCULATOR_PROGRAM_RENDER;
}

unsigned int calculator_program_task(calculator_program_t *program,
                                     unsigned int step_budget) {
    if (program->engine.state != BASIC_RUN_RUNNING) return 0;
    unsigned int revision = program->engine.output_revision;
    for (unsigned int i = 0;
         i < step_budget && program->engine.state == BASIC_RUN_RUNNING; ++i) {
        basic_engine_step(&program->engine);
    }
    if (revision == program->engine.output_revision) return 0;
    snprintf(program->notice, sizeof program->notice, "%s",
             program->engine.state == BASIC_RUN_ERROR
                 ? basic_status_text(program->engine.status)
                 : run_state_text(program));
    return CALCULATOR_PROGRAM_RENDER;
}
