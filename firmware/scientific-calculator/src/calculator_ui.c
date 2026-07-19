#include "calculator_ui.h"

#include "calculator_engine.h"
#include "calculator_complex.h"
#include "calculator_graph.h"
#include "calculator_list.h"
#include "calculator_logic.h"
#include "calculator_units.h"
#include "calculator_navigation.h"
#include "calculator_pages.h"
#include "calculator_persistence.h"
#include "calculator_storage.h"
#include "calculator_symbols.h"
#include "calculator_ui_types.h"
#include "calculator_widgets.h"
#include "expression_editor.h"
#include "number_formats.h"
#include "programmer_engine.h"
#include "board.h"
#include "lcd_st7796.h"
#include "touch_gt911.h"
#include "pico/stdlib.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define COL_BG RGB565(0, 0, 0)
#define EXPR_MAX EXPRESSION_EDITOR_CAPACITY
#define PERSISTENCE_DELAY_MS 3000u
#define PERSISTENCE_RETRY_MS 10000u

static expression_editor_t expression_state;
static char result_text[32] = "0";
static char message[24] = "READY";
static double ans;
static calc_page_t page;
static programmer_engine_t programmer;
static unsigned int fixed_fraction_bits = 16;
static calculator_format_view_t format_view;
static bool just_evaluated;
static bool touch_locked;

#define HISTORY_MAX CALCULATOR_PERSISTENCE_HISTORY_CAPACITY
typedef calculator_persisted_history_entry_t history_entry_t;

static history_entry_t history[HISTORY_MAX];
static calculator_list_t history_list;
static double memory_value;
static calculator_graph_t graph;
static calculator_symbols_t symbols;
static calculator_logic_t logic;
static calculator_units_t units;
static calculator_complex_t complex;
static bool persistence_dirty;
static absolute_time_t persistence_deadline;

static void render_display(void);
static void render_graph(void);

static void capture_persisted_state(calculator_persisted_state_t *state) {
    calculator_persistence_defaults(state);
    state->degrees = calc_engine_uses_degrees();
    state->page = page;
    state->format_bits = programmer.word_bits;
    state->fixed_fraction_bits = fixed_fraction_bits;
    state->ans = ans;
    state->memory_value = memory_value;
    state->programmer_base = programmer.base;
    state->programmer_value = programmer.value;
    state->programmer_signed = programmer.signed_mode;
    state->programmer_selected_bit = programmer.selected_bit;
    state->symbols = symbols;
    memcpy(state->history, history, sizeof history);
    state->history_count = history_list.count;
    state->history_index = history_list.index;
    state->graph = graph;
}

static void apply_persisted_state(
    const calculator_persisted_state_t *state) {
    calc_engine_set_degrees(state->degrees);
    page = state->page;
    fixed_fraction_bits = state->fixed_fraction_bits;
    ans = state->ans;
    memory_value = state->memory_value;
    symbols = state->symbols;
    memcpy(history, state->history, sizeof history);
    calculator_list_set_count(&history_list, state->history_count);
    if (state->history_count) {
        calculator_list_select(&history_list, state->history_index);
    }
    programmer.value = state->programmer_value;
    programmer.replace_input = true;
    programmer_engine_set_word_bits(&programmer, state->format_bits);
    programmer.signed_mode = state->programmer_signed;
    programmer.selected_bit = state->programmer_selected_bit;
    programmer_engine_set_base(&programmer, state->programmer_base);
    graph = state->graph;
    snprintf(result_text, sizeof result_text, "%.12g", ans);
}

static void mark_persistence_dirty(void) {
    persistence_dirty = true;
    persistence_deadline = make_timeout_time_ms(PERSISTENCE_DELAY_MS);
}

static void save_persistence_if_due(void) {
    if (!persistence_dirty || !time_reached(persistence_deadline)) return;

    calculator_persisted_state_t state;
    capture_persisted_state(&state);
    calculator_storage_save_status_t status = calculator_storage_save(&state);
    if (status == CALCULATOR_STORAGE_ERROR) {
        persistence_deadline = make_timeout_time_ms(PERSISTENCE_RETRY_MS);
        snprintf(message, sizeof message, "SAVE ERROR");
        if (page == PAGE_GRAPH) {
            render_graph();
        } else {
            render_display();
        }
        return;
    }
    persistence_dirty = false;
}

static calculator_widget_state_t current_widget_state(void) {
    unsigned int active_mask = 0;
    for (size_t i = 0; i < GRAPH_FUNCTION_COUNT; ++i) {
        if (graph.functions[i].active && graph.functions[i].expression[0]) {
            active_mask |= 1u << i;
        }
    }
    calculator_widget_state_t state = {
        .page = page,
        .programmer_base = (unsigned int)programmer.base,
        .format_bits = programmer.word_bits,
        .format_view = format_view,
        .programmer_signed = programmer.signed_mode,
        .degrees = calc_engine_uses_degrees(),
        .graph_view = graph.view,
        .graph_active_mask = active_mask,
        .graph_selected_function = graph.selected_function,
        .logic_view = logic.view,
        .logic_variable_mask = logic.compiled ? logic.program.variable_mask : 0,
        .logic_assignment = logic.assignment,
        .unit_category = units.category,
        .units_view = units.view,
        .complex_polar = complex.polar_view,
        .complex_history = complex.history_view,
    };
    for (size_t i = 0; i < CALCULATOR_FAVORITE_COUNT; ++i) {
        state.favorites[i] = symbols.favorites[i];
    }
    return state;
}

static void draw_key(const calc_key_t *key, bool pressed) {
    calculator_widget_state_t state = current_widget_state();
    calculator_widget_draw_key(key, pressed, &state);
}

static void render_display(void) {
    if (page == PAGE_PROGRAMMER) {
        calculator_page_render_programmer(&programmer, message);
        return;
    }
    if (page == PAGE_FORMAT) {
        calculator_page_render_format(&programmer, fixed_fraction_bits,
                                      format_view, message);
        return;
    }
    if (page == PAGE_TOOLS) {
        const char *formula = history_list.count
            ? history[history_list.index].formula : "";
        const char *history_result = history_list.count
            ? history[history_list.index].result : "";
        calculator_page_render_tools(memory_value, message, &expression_state,
                                     history_list.count, history_list.index,
                                     formula, history_result, result_text);
        return;
    }
    if (page == PAGE_SYMBOLS) {
        calculator_page_render_symbols(&symbols, graph.selected_function,
                                       message);
        return;
    }
    if (page == PAGE_LOGIC) {
        calculator_page_render_logic(&logic, message);
        return;
    }
    if (page == PAGE_UNITS) {
        calculator_page_render_units(&units, message);
        return;
    }
    if (page == PAGE_COMPLEX) {
        calculator_page_render_complex(&complex, calc_engine_uses_degrees(),
                                       message);
        return;
    }
    if (page == PAGE_GRAPH) return;
    calculator_page_render_expression(page, calc_engine_uses_degrees(),
                                      message, &expression_state, result_text);
}

static void render_keypad(void) {
    calculator_widget_state_t state = current_widget_state();
    calculator_widget_render_keypad(page, &state);
}

static void render_graph(void) {
    calculator_widget_state_t state = current_widget_state();
    calculator_graph_render(&graph, ans, &symbols, &state, message,
                            sizeof message);
}

static bool token_is_operator(const char *token) {
    return token && token[0] && token[1] == '\0' && strchr("+-*/^%", token[0]);
}

static void insert_token(const char *token) {
    if (!token || !*token) return;

    if (just_evaluated) {
        if (token_is_operator(token)) {
            expression_editor_set(&expression_state, "ans");
        } else {
            expression_editor_clear(&expression_state);
        }
        just_evaluated = false;
    }

    if (!expression_editor_insert(&expression_state, token)) {
        snprintf(message, sizeof message, "INPUT FULL");
        return;
    }
    snprintf(message, sizeof message, "READY");
}

static void delete_expression_char(void) {
    expression_editor_delete(&expression_state);
    just_evaluated = false;
}

static void add_history(double value) {
    if (!expression_state.length) return;
    if (history_list.count == HISTORY_MAX) {
        memmove(&history[0], &history[1],
                (HISTORY_MAX - 1) * sizeof history[0]);
        calculator_list_set_count(&history_list, HISTORY_MAX - 1);
    }
    size_t slot = history_list.count;
    snprintf(history[slot].formula,
             sizeof history[slot].formula, "%s",
             expression_state.text);
    snprintf(history[slot].result,
             sizeof history[slot].result, "%s", result_text);
    history[slot].value = value;
    calculator_list_set_count(&history_list, slot + 1);
    calculator_list_select(&history_list, slot);
}

static void evaluate_expression(void) {
    double value = 0.0;
    int error_position = 0;
    calc_status_t status = calc_engine_evaluate_symbols(
        expression_state.text, ans, &symbols, &value, &error_position);
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
                snprintf(message, sizeof message, "%u-BIT OVERFLOW",
                         programmer.word_bits);
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
    programmer_engine_set_value(&programmer, value);
}

static void activate_format_key(const calc_key_t *key) {
    unsigned int format_bits = programmer.word_bits;
    uint64_t mask = number_format_mask(format_bits);

    if (key->action == ACT_FMT_VIEW) {
        if (strcmp(key->token, "BITS") == 0) {
            format_view = FORMAT_VIEW_BITS;
        } else if (strcmp(key->token, "IEEE32") == 0) {
            format_view = FORMAT_VIEW_IEEE32;
        } else if (strcmp(key->token, "IEEE64") == 0) {
            format_view = FORMAT_VIEW_IEEE64;
        } else {
            format_view = FORMAT_VIEW_CONVERSIONS;
        }
        snprintf(message, sizeof message, "%s VIEW",
                 format_view == FORMAT_VIEW_BITS ? "BIT" :
                 (format_view == FORMAT_VIEW_IEEE32 ? "IEEE32" :
                  (format_view == FORMAT_VIEW_IEEE64 ? "IEEE64" :
                   "FORMAT")));
        render_keypad();
        return;
    }

    if (key->action == ACT_FMT_WIDTH) {
        unsigned int bits = key->token[0] == '8' ? 8u :
            (key->token[0] == '1' ? 16u : (key->token[0] == '3' ? 32u : 64u));
        programmer_engine_set_word_bits(&programmer, bits);
        if (fixed_fraction_bits >= bits) fixed_fraction_bits = bits - 1u;
        snprintf(message, sizeof message, "%u-BIT WORD", bits);
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
        programmer_engine_negate(&programmer);
        snprintf(message, sizeof message, "TWOS COMPLEMENT");
    } else if (strcmp(key->token, "MASK") == 0) {
        set_programmer_value(programmer.value & mask);
        snprintf(message, sizeof message, "MASK APPLIED");
    } else if (strcmp(key->token, "SEXT") == 0) {
        uint64_t extended = number_format_sign_extend(programmer.value,
                                                       format_bits);
        programmer_engine_set_word_bits(&programmer, 64);
        set_programmer_value(extended);
        snprintf(message, sizeof message, "SIGN EXTENDED");
        render_keypad();
    } else if (strcmp(key->token, "Q-") == 0) {
        if (fixed_fraction_bits > 0) fixed_fraction_bits--;
        snprintf(message, sizeof message, "Q FRACTION -1");
    } else if (strcmp(key->token, "Q+") == 0) {
        if (fixed_fraction_bits + 1u < programmer.word_bits) fixed_fraction_bits++;
        snprintf(message, sizeof message, "Q FRACTION +1");
    } else if (strcmp(key->token, "Q0") == 0) {
        fixed_fraction_bits = 0;
        snprintf(message, sizeof message, "Q0 FORMAT");
    } else if (strcmp(key->token, "Q8") == 0 ||
               strcmp(key->token, "Q16") == 0 ||
               strcmp(key->token, "Q24") == 0) {
        unsigned int requested = key->token[1] == '8' ? 8u :
            (key->token[1] == '1' ? 16u : 24u);
        fixed_fraction_bits = requested < programmer.word_bits
            ? requested : programmer.word_bits - 1u;
        snprintf(message, sizeof message, "Q%u FORMAT", fixed_fraction_bits);
    } else if (strcmp(key->token, "A32") == 0) {
        programmer_engine_set_word_bits(&programmer, 32);
        set_programmer_value(number_format_float32_bits(ans));
        programmer_engine_set_base(&programmer, PROGRAMMER_HEX);
        snprintf(message, sizeof message, "ANS TO FLOAT32");
        render_keypad();
    } else if (strcmp(key->token, "A64") == 0) {
        programmer_engine_set_word_bits(&programmer, 64);
        set_programmer_value(number_format_float64_bits(ans));
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
        set_programmer_value(UINT64_C(1) << (programmer.word_bits - 1u));
        snprintf(message, sizeof message, "SIGNED MIN");
    } else if (strcmp(key->token, "MAX") == 0) {
        set_programmer_value(
            (UINT64_C(1) << (programmer.word_bits - 1u)) - 1u);
        snprintf(message, sizeof message, "SIGNED MAX");
    } else if (strcmp(key->token, "INC") == 0) {
        programmer_engine_increment(&programmer);
        snprintf(message, sizeof message, "+1");
    } else if (strcmp(key->token, "DEC") == 0) {
        programmer_engine_decrement(&programmer);
        snprintf(message, sizeof message, "-1");
    } else if (strcmp(key->token, "SHL") == 0) {
        programmer_engine_shift_left(&programmer);
        snprintf(message, sizeof message, "SHIFT LEFT");
    } else if (strcmp(key->token, "SHR") == 0) {
        programmer_engine_shift_right(&programmer);
        snprintf(message, sizeof message, "SHIFT RIGHT");
    } else if (strcmp(key->token, "ROL") == 0) {
        programmer_engine_rotate_left(&programmer);
        snprintf(message, sizeof message, "ROTATE LEFT");
    } else if (strcmp(key->token, "ROR") == 0) {
        programmer_engine_rotate_right(&programmer);
        snprintf(message, sizeof message, "ROTATE RIGHT");
    } else if (strcmp(key->token, "SWAP") == 0) {
        programmer_engine_swap_endian(&programmer);
        snprintf(message, sizeof message, "BYTE ORDER SWAPPED");
    } else if (strcmp(key->token, "SIGN") == 0) {
        programmer_engine_toggle_signed(&programmer);
        snprintf(message, sizeof message, "%s MODE",
                 programmer.signed_mode ? "SIGNED" : "UNSIGNED");
        render_keypad();
    } else if (strcmp(key->token, "BIT-8") == 0) {
        programmer_engine_select_bit(&programmer, -8);
        snprintf(message, sizeof message, "BIT %u", programmer.selected_bit);
    } else if (strcmp(key->token, "BIT-1") == 0) {
        programmer_engine_select_bit(&programmer, -1);
        snprintf(message, sizeof message, "BIT %u", programmer.selected_bit);
    } else if (strcmp(key->token, "BIT+1") == 0) {
        programmer_engine_select_bit(&programmer, 1);
        snprintf(message, sizeof message, "BIT %u", programmer.selected_bit);
    } else if (strcmp(key->token, "BIT+8") == 0) {
        programmer_engine_select_bit(&programmer, 8);
        snprintf(message, sizeof message, "BIT %u", programmer.selected_bit);
    } else if (strcmp(key->token, "BSET") == 0) {
        programmer_engine_set_selected_bit(&programmer);
        snprintf(message, sizeof message, "BIT SET");
    } else if (strcmp(key->token, "BCLR") == 0) {
        programmer_engine_clear_selected_bit(&programmer);
        snprintf(message, sizeof message, "BIT CLEARED");
    } else if (strcmp(key->token, "BTOGGLE") == 0) {
        programmer_engine_toggle_selected_bit(&programmer);
        snprintf(message, sizeof message, "BIT TOGGLED");
    } else if (strcmp(key->token, "FLAGS") == 0) {
        programmer_engine_clear_flags(&programmer);
        snprintf(message, sizeof message, "FLAGS CLEARED");
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
        memset(history, 0, sizeof history);
        calculator_list_set_count(&history_list, 0);
        snprintf(message, sizeof message, "HISTORY CLEARED");
        return;
    }
    if (calculator_list_empty(&history_list)) {
        snprintf(message, sizeof message, "NO HISTORY");
        return;
    }
    if (strcmp(key->token, "PREV") == 0) {
        calculator_list_previous(&history_list);
        snprintf(message, sizeof message, "OLDER RESULT");
    } else if (strcmp(key->token, "NEXT") == 0) {
        calculator_list_next(&history_list);
        snprintf(message, sizeof message, "NEWER RESULT");
    } else {
        expression_editor_set(&expression_state,
                              history[history_list.index].formula);
        snprintf(result_text, sizeof result_text, "%s",
                 history[history_list.index].result);
        ans = history[history_list.index].value;
        just_evaluated = false;
        page = PAGE_BASIC;
        snprintf(message, sizeof message, "HISTORY LOADED");
        render_keypad();
    }
}

static void activate_cursor_key(const calc_key_t *key) {
    editor_cursor_action_t action = EDITOR_CURSOR_END;
    if (strcmp(key->token, "LEFT") == 0) {
        action = EDITOR_CURSOR_LEFT;
    } else if (strcmp(key->token, "RIGHT") == 0) {
        action = EDITOR_CURSOR_RIGHT;
    } else if (strcmp(key->token, "HOME") == 0) {
        action = EDITOR_CURSOR_HOME;
    }
    expression_editor_move(&expression_state, action);
    snprintf(message, sizeof message, "CURSOR %u",
             (unsigned int)expression_state.cursor);
}

static bool save_function(size_t function, const char *expression) {
    calculator_symbols_t candidate = symbols;
    calculator_symbol_status_t symbol_status =
        calculator_symbols_set_function(&candidate, function, expression);
    if (symbol_status != CALCULATOR_SYMBOL_OK) {
        snprintf(message, sizeof message, "%s",
                 calculator_symbol_status_text(symbol_status));
        return false;
    }

    size_t invalid_function = CALCULATOR_USER_FUNCTION_COUNT;
    int error_position = 0;
    if (calc_engine_validate_symbols(&candidate, &invalid_function,
                                     &error_position) != CALC_OK) {
        size_t shown_function = invalid_function < CALCULATOR_USER_FUNCTION_COUNT
            ? invalid_function : function;
        if (error_position > 0) {
            snprintf(message, sizeof message, "F%u SYNTAX %d",
                     (unsigned int)(shown_function + 1), error_position);
        } else {
            snprintf(message, sizeof message, "F%u INVALID DEF",
                     (unsigned int)(shown_function + 1));
        }
        return false;
    }

    symbols = candidate;
    graph_model_set_function(&graph, function, symbols.functions[function]);
    snprintf(message, sizeof message, "F%u SAVED",
             (unsigned int)(function + 1));
    return true;
}

static void activate_symbol_key(const calc_key_t *key) {
    size_t index;
    switch (key->action) {
        case ACT_SYMBOL_STORE:
            index = (size_t)(key->token[0] - 'A');
            if (calculator_symbols_set_variable(&symbols, index, ans)) {
                snprintf(message, sizeof message, "%s = %.10g",
                         calculator_variable_name(index), ans);
            } else {
                snprintf(message, sizeof message, "STORE FAILED");
            }
            break;
        case ACT_SYMBOL_FUNCTION:
            index = (size_t)(key->token[0] - '0');
            graph_model_select_function(&graph, index);
            snprintf(message, sizeof message, "F%u SELECTED",
                     (unsigned int)(index + 1));
            break;
        case ACT_SYMBOL_EDIT:
            expression_editor_set(
                &expression_state,
                symbols.functions[graph.selected_function]);
            just_evaluated = false;
            page = PAGE_TOOLS;
            snprintf(message, sizeof message, "EDIT F%u",
                     (unsigned int)(graph.selected_function + 1));
            render_keypad();
            break;
        case ACT_SYMBOL_SAVE:
            if (save_function(graph.selected_function,
                              expression_state.text)) {
                just_evaluated = true;
            }
            break;
        case ACT_FAVORITE:
            index = (size_t)(key->token[0] - '0');
            if (symbols.favorites[index][0]) {
                insert_token(symbols.favorites[index]);
                page = PAGE_TOOLS;
                snprintf(message, sizeof message, "FAVORITE %u",
                         (unsigned int)(index + 1));
                render_keypad();
            } else {
                snprintf(message, sizeof message, "FAVORITE %u EMPTY",
                         (unsigned int)(index + 1));
            }
            break;
        case ACT_FAVORITE_SET:
            index = (size_t)(key->token[0] - '0');
            if (!expression_state.length) {
                snprintf(message, sizeof message, "EXPRESSION EMPTY");
                break;
            }
            calculator_symbol_status_t status =
                calculator_symbols_set_favorite(&symbols, index,
                                                 expression_state.text);
            if (status == CALCULATOR_SYMBOL_OK) {
                just_evaluated = true;
                snprintf(message, sizeof message, "FAVORITE %u SET",
                         (unsigned int)(index + 1));
                render_keypad();
            } else {
                snprintf(message, sizeof message, "%s",
                         calculator_symbol_status_text(status));
            }
            break;
        default:
            break;
    }
}

static void activate_graph_key(const calc_key_t *key) {
    if (key->token[0] == 'F' && key->token[1] >= '1' &&
        key->token[1] <= '3' && key->token[2] == '\0') {
        size_t index = (size_t)(key->token[1] - '1');
        graph_model_select_function(&graph, index);
        expression_editor_set(&expression_state,
                              graph.functions[index].expression);
        snprintf(message, sizeof message, "F%u SELECTED",
                 (unsigned int)(index + 1));
    } else if (strcmp(key->token, "TOGGLE") == 0) {
        if (graph_model_toggle_function(&graph, graph.selected_function)) {
            snprintf(message, sizeof message, "F%u %s",
                     (unsigned int)(graph.selected_function + 1),
                     graph.functions[graph.selected_function].active
                        ? "ON" : "OFF");
        } else {
            snprintf(message, sizeof message, "EDIT F%u IN TOOLS",
                     (unsigned int)(graph.selected_function + 1));
        }
    } else if (strcmp(key->token, "MENU") == 0) {
        graph_model_set_view(&graph, GRAPH_VIEW_MENU);
    } else if (strcmp(key->token, "ANALYZE") == 0) {
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS);
    } else if (strcmp(key->token, "ANMORE") == 0) {
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS_MORE);
    } else if (strcmp(key->token, "ANBACK") == 0) {
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS);
    } else if (strcmp(key->token, "PLOT") == 0) {
        graph_model_set_view(&graph, GRAPH_VIEW_PLOT);
    } else if (strcmp(key->token, "TABLE") == 0) {
        graph_model_set_view(&graph, GRAPH_VIEW_TABLE);
    } else if (strcmp(key->token, "RANGE") == 0) {
        graph_model_set_view(&graph, GRAPH_VIEW_RANGE);
    } else if (strcmp(key->token, "TRACE") == 0) {
        graph_model_clear_analysis(&graph);
        graph.trace_enabled = !graph.trace_enabled;
        if (graph.trace_enabled) graph.trace_x = graph.x_center;
        graph_model_set_view(&graph, GRAPH_VIEW_PLOT);
        snprintf(message, sizeof message, "TRACE %s",
                 graph.trace_enabled ? "ON" : "OFF");
    } else if (strcmp(key->token, "AUTO") == 0) {
        calc_status_t status = calculator_graph_auto_scale(&graph, ans,
                                                           &symbols);
        snprintf(message, sizeof message, "%s",
                 status == CALC_OK ? "AUTO SCALE" :
                 calc_engine_status_text(status));
        graph_model_set_view(&graph, GRAPH_VIEW_PLOT);
    } else if (strcmp(key->token, "ROOT") == 0) {
        calculator_graph_analyze(&graph, ans, &symbols,
                                 CALCULATOR_GRAPH_ROOT);
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS);
    } else if (strcmp(key->token, "XING") == 0) {
        calculator_graph_analyze(&graph, ans, &symbols,
                                 CALCULATOR_GRAPH_INTERSECTION);
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS);
    } else if (strcmp(key->token, "DERIV") == 0) {
        calculator_graph_analyze(&graph, ans, &symbols,
                                 CALCULATOR_GRAPH_DERIVATIVE);
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS);
    } else if (strcmp(key->token, "INTEGR") == 0) {
        calculator_graph_analyze(&graph, ans, &symbols,
                                 CALCULATOR_GRAPH_INTEGRAL);
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS);
    } else if (strcmp(key->token, "EXTREM") == 0) {
        calculator_graph_analyze(&graph, ans, &symbols,
                                 CALCULATOR_GRAPH_EXTREMA);
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS);
    } else if (strcmp(key->token, "A=ANS") == 0) {
        graph_model_set_analysis_bound(&graph, true, ans);
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS_MORE);
    } else if (strcmp(key->token, "B=ANS") == 0) {
        graph_model_set_analysis_bound(&graph, false, ans);
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS_MORE);
    } else if (strcmp(key->token, "VIEWINT") == 0) {
        graph_model_use_view_interval(&graph);
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS_MORE);
    } else if (strcmp(key->token, "TOL") == 0) {
        graph_model_cycle_analysis_tolerance(&graph);
        graph_model_set_view(&graph, GRAPH_VIEW_ANALYSIS_MORE);
    } else if (strcmp(key->token, "IN") == 0) {
        calculator_graph_zoom(&graph, 0.5);
    } else if (strcmp(key->token, "TABLE-") == 0) {
        graph_model_scroll_table(&graph, -5.0);
    } else if (strcmp(key->token, "TABLE+") == 0) {
        graph_model_scroll_table(&graph, 5.0);
    } else if (strcmp(key->token, "STEP-") == 0) {
        graph_model_scale_table_step(&graph, 0.5);
    } else if (strcmp(key->token, "STEP+") == 0) {
        graph_model_scale_table_step(&graph, 2.0);
    } else if (strcmp(key->token, "XSPAN-") == 0 ||
               strcmp(key->token, "XSPAN+") == 0) {
        double factor = key->token[5] == '-' ? 0.5 : 2.0;
        graph_model_set_range(&graph,
            graph.x_center - graph.x_span * factor * 0.5,
            graph.x_center + graph.x_span * factor * 0.5,
            graph.y_center - graph.y_span * 0.5,
            graph.y_center + graph.y_span * 0.5);
        graph_model_set_view(&graph, GRAPH_VIEW_RANGE);
    } else if (strcmp(key->token, "YSPAN-") == 0 ||
               strcmp(key->token, "YSPAN+") == 0) {
        double factor = key->token[5] == '-' ? 0.5 : 2.0;
        graph_model_set_range(&graph,
            graph.x_center - graph.x_span * 0.5,
            graph.x_center + graph.x_span * 0.5,
            graph.y_center - graph.y_span * factor * 0.5,
            graph.y_center + graph.y_span * factor * 0.5);
        graph_model_set_view(&graph, GRAPH_VIEW_RANGE);
    } else if (strcmp(key->token, "RESET") == 0) {
        graph_model_reset_range(&graph);
        graph_model_set_view(&graph, GRAPH_VIEW_RANGE);
    }
    render_graph();
}

static void activate_logic_key(const calc_key_t *key) {
    calculator_logic_activate(&logic, key->token, message, sizeof message);
    render_keypad();
}

static void activate_units_key(const calc_key_t *key) {
    double output = 0.0;
    calculator_units_output_t action = calculator_units_activate(
        &units, key->token, ans, &output, message, sizeof message);
    if (action == UNITS_OUTPUT_ANS) {
        ans = output;
        snprintf(result_text, sizeof result_text, "%.12g", ans);
        snprintf(message, sizeof message, "VALUE TO ANS");
    } else if (action == UNITS_OUTPUT_EDITOR) {
        char token[32];
        snprintf(token, sizeof token, "%.12g", output);
        insert_token(token);
        if (strcmp(message, "INPUT FULL") != 0) {
            page = PAGE_BASIC;
            snprintf(message, sizeof message, "VALUE TO EDITOR");
        }
    }
    render_keypad();
}

static void activate_complex_key(const calc_key_t *key) {
    calculator_complex_activate(&complex, key->token,
                                calc_engine_uses_degrees(),
                                message, sizeof message);
    render_keypad();
}

static void activate_key(const calc_key_t *key) {
    switch (key->action) {
        case ACT_INSERT:
            insert_token(key->token);
            if (page == PAGE_SYMBOLS) {
                page = PAGE_TOOLS;
                render_keypad();
            }
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
                expression_editor_clear(&expression_state);
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
            page = calculator_navigation_next(page);
            snprintf(message, sizeof message, "%s", calculator_page_message(page));
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
            snprintf(message, sizeof message, "%s", calculator_page_message(page));
            render_keypad();
            break;
        case ACT_FMT_WIDTH:
        case ACT_FMT_ACTION:
        case ACT_FMT_VIEW:
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
        case ACT_SYMBOL_STORE:
        case ACT_SYMBOL_FUNCTION:
        case ACT_SYMBOL_EDIT:
        case ACT_SYMBOL_SAVE:
        case ACT_FAVORITE:
        case ACT_FAVORITE_SET:
            activate_symbol_key(key);
            break;
        case ACT_GOTO_GRAPH:
            if (expression_state.length) {
                if (!save_function(graph.selected_function,
                                   expression_state.text)) {
                    break;
                }
                calculator_graph_auto_scale(&graph, ans, &symbols);
            }
            page = PAGE_GRAPH;
            graph_model_set_view(&graph, GRAPH_VIEW_PLOT);
            render_graph();
            break;
        case ACT_GOTO_TOOLS:
            expression_editor_set(
                &expression_state,
                symbols.functions[graph.selected_function]);
            page = PAGE_TOOLS;
            snprintf(message, sizeof message, "%s", calculator_page_message(page));
            render_keypad();
            break;
        case ACT_GRAPH:
            activate_graph_key(key);
            break;
        case ACT_LOGIC:
            activate_logic_key(key);
            break;
        case ACT_UNITS:
            activate_units_key(key);
            break;
        case ACT_COMPLEX:
            activate_complex_key(key);
            break;
    }
    render_display();
    mark_persistence_dirty();
}

static const calc_key_t *hit_key(uint16_t x, uint16_t y) {
    calculator_widget_state_t state = current_widget_state();
    return calculator_widget_hit_key(page, &state, x, y);
}

void calculator_ui_init(void) {
    lcd_fill(COL_BG);
    expression_editor_init(&expression_state);
    calculator_list_init(&history_list);
    programmer_engine_init(&programmer);
    calculator_logic_init(&logic);
    calculator_units_init(&units);
    calculator_complex_init(&complex);

    calculator_persisted_state_t persisted;
    calculator_storage_load_status_t load_status =
        calculator_storage_load(&persisted);
    bool factory_reset = board_button1_pressed() && board_button2_pressed();
    if (factory_reset) {
        calculator_persistence_defaults(&persisted);
        (void)calculator_storage_save(&persisted);
        while (board_button1_pressed() || board_button2_pressed()) {
            sleep_ms(10);
        }
    }
    apply_persisted_state(&persisted);
    if (!touch_is_ready()) {
        snprintf(message, sizeof message, "TOUCH ERROR");
    } else if (factory_reset) {
        snprintf(message, sizeof message, "FACTORY RESET");
    } else if (load_status == CALCULATOR_STORAGE_RECOVERED) {
        snprintf(message, sizeof message, "STORAGE RECOVERED");
        mark_persistence_dirty();
    } else if (load_status == CALCULATOR_STORAGE_LOADED) {
        snprintf(message, sizeof message, "STATE RESTORED");
    } else {
        snprintf(message, sizeof message, "READY");
    }
    if (page == PAGE_GRAPH) {
        render_graph();
    } else {
        render_display();
        render_keypad();
    }
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
        } else if (page == PAGE_COMPLEX) {
            calculator_complex_activate(&complex, "=",
                                        calc_engine_uses_degrees(),
                                        message, sizeof message);
            render_keypad();
        } else if (calculator_page_accepts_evaluate(page)) {
            evaluate_expression();
        }
        render_display();
        mark_persistence_dirty();
    }
    if (button2 && !old_button2) {
        if (page == PAGE_PROGRAMMER) {
            programmer_engine_delete(&programmer);
        } else if (page == PAGE_COMPLEX) {
            calculator_complex_activate(&complex, "DEL",
                                        calc_engine_uses_degrees(),
                                        message, sizeof message);
            render_keypad();
        } else if (page == PAGE_LOGIC) {
            calculator_logic_activate(&logic, "DEL",
                                      message, sizeof message);
            render_keypad();
        } else if (calculator_page_accepts_expression(page)) {
            delete_expression_char();
        }
        render_display();
        mark_persistence_dirty();
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
            if (graph.view == GRAPH_VIEW_TABLE) {
                if (joystick.up) graph_model_scroll_table(&graph, -1.0);
                if (joystick.down) graph_model_scroll_table(&graph, 1.0);
            } else if (graph.trace_enabled &&
                       (joystick.left || joystick.right)) {
                graph_model_move_trace(&graph, joystick.left ? -1.0 : 1.0);
            } else {
                double horizontal = joystick.left ? -0.1 :
                    (joystick.right ? 0.1 : 0.0);
                double vertical = joystick.up ? 0.1 :
                    (joystick.down ? -0.1 : 0.0);
                calculator_graph_pan(&graph, horizontal, vertical);
            }
            render_graph();
            mark_persistence_dirty();
        } else if (page == PAGE_COMPLEX) {
            if (complex.history_view &&
                (joystick.left || joystick.right)) {
                calculator_complex_activate(
                    &complex, joystick.left ? "PREV" : "NEXT",
                    calc_engine_uses_degrees(), message, sizeof message);
            } else {
                if (joystick.left) {
                    expression_editor_move(&complex.editor,
                                           EDITOR_CURSOR_LEFT);
                }
                if (joystick.right) {
                    expression_editor_move(&complex.editor,
                                           EDITOR_CURSOR_RIGHT);
                }
            }
            render_display();
        } else if (page == PAGE_LOGIC) {
            const char *token = joystick.left ? "LEFT" :
                (joystick.right ? "RIGHT" :
                 (joystick.up ? "UP" : "DOWN"));
            calculator_logic_activate(&logic, token,
                                      message, sizeof message);
            render_display();
        } else if (calculator_page_accepts_expression(page)) {
            if (joystick.left) {
                expression_editor_move(&expression_state, EDITOR_CURSOR_LEFT);
            }
            if (joystick.right) {
                expression_editor_move(&expression_state, EDITOR_CURSOR_RIGHT);
            }
            render_display();
        }
    }
    save_persistence_if_due();
}
