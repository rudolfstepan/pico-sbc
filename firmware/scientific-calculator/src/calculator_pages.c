#include "calculator_pages.h"

#include "calculator_engine.h"
#include "calculator_widgets.h"
#include "lcd_st7796.h"
#include "number_formats.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define COL_BG    RGB565(0, 0, 0)
#define COL_TEXT  RGB565(255, 255, 255)
#define COL_MUTED RGB565(170, 170, 170)

static int display_y(int logical_y) {
    if (calculator_widget_fullscreen()) {
        return logical_y > 4 ? 4 + (logical_y - 4) * 4 : logical_y;
    }
    return calculator_widget_data_focus() ? logical_y * 2 : logical_y;
}

static void page_draw_text(int x, int y, const char *text,
                           uint16_t foreground, uint16_t background,
                           uint8_t scale) {
    if (calculator_widget_layout() != CALCULATOR_LAYOUT_STANDARD && y > 4) {
        uint8_t enlarged = scale < 2 ? 2 : (scale < 3 ? 3 : scale);
        size_t max_chars = (size_t)(LCD_WIDTH - x) /
                           ((size_t)enlarged * 6u);
        text = calculator_widget_tail(text, max_chars);
        scale = enlarged;
    }
    lcd_draw_text(x, display_y(y), text, foreground, background, scale);
}

static void page_draw_text_absolute(int x, int y, const char *text,
                                    uint16_t foreground,
                                    uint16_t background, uint8_t scale) {
    lcd_draw_text(x, y, text, foreground, background, scale);
}

/* Page renderers retain their 84-pixel logical coordinates in every layout. */
#define lcd_draw_text page_draw_text

static void clear_display(void) {
    lcd_fill_rect(0, 0, LCD_WIDTH,
                  calculator_widget_display_height(), COL_BG);
}

static void finish_display(void) {
    lcd_fill_rect(0, calculator_widget_display_height() - 2,
                  LCD_WIDTH, 2, COL_MUTED);
}

void calculator_page_render_expression(calc_page_t page, bool degrees,
                                       const char *message,
                                       const expression_editor_t *editor,
                                       const char *result_text) {
    char status[48];
    char editor_text[EXPRESSION_EDITOR_CAPACITY + 2];
    char shown_result[CALCULATOR_RESULT_TEXT_CAPACITY + 2u];

    snprintf(status, sizeof status, "%s  %s  %s",
             degrees ? "DEG" : "RAD",
             page == PAGE_SCIENTIFIC ? "SCIENTIFIC" : "BASIC", message);
    snprintf(shown_result, sizeof shown_result, "=%s", result_text);
    bool enlarged = calculator_widget_layout() != CALCULATOR_LAYOUT_STANDARD;
    size_t visible_chars = enlarged ? 26u : 38u;
    int result_scale = enlarged ? 3 : 2;
    const char *visible = calculator_widget_tail(shown_result, visible_chars);
    int width = (int)strlen(visible) * 6 * result_scale;
    int x = LCD_WIDTH - width - 6;
    if (x < 6) x = 6;

    clear_display();
    lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 20,
                  expression_editor_view(editor, editor_text,
                                         sizeof editor_text, visible_chars),
                  COL_TEXT, COL_BG, 2);
    lcd_draw_text(x, 52, visible, COL_TEXT, COL_BG, 2);
    finish_display();
}

void calculator_page_render_programmer(const programmer_engine_t *programmer,
                                       const char *message) {
    char status[64];
    char decimal[21];
    char hexadecimal[17];
    char binary[65];
    char conversions[56];
    char binary_line[72];

    uint64_t raw = number_format_apply_width(programmer->value,
                                             programmer->word_bits);
    programmer_engine_format(raw, PROGRAMMER_DEC,
                             decimal, sizeof decimal);
    programmer_engine_format(raw, PROGRAMMER_HEX,
                             hexadecimal, sizeof hexadecimal);
    programmer_engine_format(raw, PROGRAMMER_BIN,
                             binary, sizeof binary);
    if (programmer->signed_mode) {
        number_format_signed_text(raw, programmer->word_bits,
                                  decimal, sizeof decimal);
    }
    snprintf(status, sizeof status, "PROGRAMMER %s%u C%u V%u %s %s%s%s",
             programmer->signed_mode ? "S" : "U", programmer->word_bits,
             programmer->carry ? 1u : 0u, programmer->overflow ? 1u : 0u,
             programmer_engine_base_name(programmer->base),
             programmer_engine_op_name(programmer->pending_op),
             programmer->pending_op == PROGRAMMER_OP_NONE ? "" : "  ",
             message);
    snprintf(conversions, sizeof conversions, "%s %s  HEX %s",
             programmer->signed_mode ? "SIGNED" : "UNSIGNED",
             decimal, hexadecimal);
    snprintf(binary_line, sizeof binary_line, "BIN %s", binary);

    clear_display();
    lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 18, calculator_widget_tail(programmer->input, 38),
                  COL_TEXT, COL_BG, 2);
    lcd_draw_text(6, 45, conversions, COL_TEXT, COL_BG, 1);
    lcd_draw_text(6, 62, binary_line, COL_MUTED, COL_BG, 1);
    finish_display();
}

void calculator_page_render_format(const programmer_engine_t *programmer,
                                   unsigned int fixed_fraction_bits,
                                   calculator_format_view_t view,
                                   const char *message) {
    unsigned int format_bits = programmer->word_bits;
    uint64_t raw = number_format_apply_width(programmer->value, format_bits);
    char decimal[21];
    char hexadecimal[17];
    char binary[65];
    char signed_text[24];
    char status[48];
    char raw_line[24];
    char fixed_line[52];
    char float_line[72];

    programmer_engine_format(raw, PROGRAMMER_DEC,
                             decimal, sizeof decimal);
    programmer_engine_format(raw, PROGRAMMER_HEX,
                             hexadecimal, sizeof hexadecimal);
    programmer_engine_format(raw, PROGRAMMER_BIN,
                             binary, sizeof binary);
    number_format_signed_text(raw, format_bits,
                              signed_text, sizeof signed_text);
    double fixed = number_format_fixed_value(raw, format_bits,
                                             fixed_fraction_bits);
    double float32 = number_format_bits_float32((uint32_t)raw);
    double float64 = number_format_bits_float64(raw);

    if (view == FORMAT_VIEW_BITS) {
        char bit_line[32];
        snprintf(status, sizeof status, "BITS %s%u BIT%u=%u C%u V%u %s",
                 programmer->signed_mode ? "S" : "U", format_bits,
                 programmer->selected_bit,
                 programmer_engine_selected_bit(programmer) ? 1u : 0u,
                 programmer->carry ? 1u : 0u,
                 programmer->overflow ? 1u : 0u, message);
        snprintf(bit_line, sizeof bit_line, "BIT %u = %u",
                 programmer->selected_bit,
                 programmer_engine_selected_bit(programmer) ? 1u : 0u);
        snprintf(fixed_line, sizeof fixed_line, "U %s  S %s",
                 decimal, signed_text);
        snprintf(float_line, sizeof float_line, "BIN %s", binary);

        clear_display();
        lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
        lcd_draw_text(6, 18, bit_line, COL_TEXT, COL_BG, 2);
        lcd_draw_text(6, 47, fixed_line, COL_TEXT, COL_BG, 1);
        lcd_draw_text(6, 64, float_line, COL_MUTED, COL_BG, 1);
        finish_display();
        return;
    }

    if (view == FORMAT_VIEW_IEEE32 || view == FORMAT_VIEW_IEEE64) {
        bool use_float32 = view == FORMAT_VIEW_IEEE32;
        number_format_ieee_t ieee = use_float32
            ? number_format_inspect_float32((uint32_t)raw)
            : number_format_inspect_float64(raw);
        double value = use_float32
            ? number_format_bits_float32((uint32_t)raw)
            : number_format_bits_float64(raw);
        char fields[40];
        char ieee_hex[17];
        unsigned int mantissa_digits = use_float32 ? 6u : 13u;
        programmer_engine_format(use_float32 ? (uint32_t)raw : raw,
                                 PROGRAMMER_HEX, ieee_hex, sizeof ieee_hex);
        snprintf(status, sizeof status, "IEEE F%u %s %s",
                 use_float32 ? 32u : 64u,
                 number_format_ieee_class_name(ieee.classification), message);
        snprintf(fields, sizeof fields, "S%u E%u X%d",
                 ieee.sign ? 1u : 0u, ieee.raw_exponent,
                 ieee.unbiased_exponent);
        snprintf(fixed_line, sizeof fixed_line, "M 0x%0*llX",
                 (int)mantissa_digits, (unsigned long long)ieee.mantissa);
        snprintf(float_line, sizeof float_line, "RAW %s  VAL %.10g",
                 ieee_hex, value);

        clear_display();
        lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
        lcd_draw_text(6, 18, fields, COL_TEXT, COL_BG, 2);
        lcd_draw_text(6, 47, fixed_line, COL_TEXT, COL_BG, 1);
        lcd_draw_text(6, 64, float_line, COL_MUTED, COL_BG, 1);
        finish_display();
        return;
    }

    snprintf(status, sizeof status, "FORMAT  %uBIT  Q%u  %s",
             format_bits, fixed_fraction_bits, message);
    snprintf(raw_line, sizeof raw_line, "RAW %s", hexadecimal);
    snprintf(fixed_line, sizeof fixed_line, "TC %s  FIX %.10g",
             signed_text, fixed);
    snprintf(float_line, sizeof float_line, "F32 %.8g  F64 %.10g",
             float32, float64);

    clear_display();
    lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 18, raw_line, COL_TEXT, COL_BG, 2);
    lcd_draw_text(6, 47, fixed_line, COL_TEXT, COL_BG, 1);
    lcd_draw_text(6, 64, float_line, COL_MUTED, COL_BG, 1);
    finish_display();
}

void calculator_page_render_tools(double memory_value, const char *message,
                                  const expression_editor_t *editor,
                                  size_t history_count,
                                  size_t history_index,
                                  const char *history_formula,
                                  const char *history_result,
                                  const char *result_text) {
    char status[48];
    char editor_text[EXPRESSION_EDITOR_CAPACITY + 2];
    char history_line[80];
    char result_line[48];

    snprintf(status, sizeof status, "TOOLS  M %.10g  %s", memory_value, message);
    if (history_count) {
        snprintf(history_line, sizeof history_line, "H%u %s",
                 (unsigned int)(history_index + 1), history_formula);
        snprintf(result_line, sizeof result_line, "RESULT %s", history_result);
    } else {
        snprintf(history_line, sizeof history_line, "NO HISTORY");
        snprintf(result_line, sizeof result_line, "RESULT %s", result_text);
    }

    clear_display();
    lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 18,
                  expression_editor_view(editor, editor_text,
                                         sizeof editor_text, 38),
                  COL_TEXT, COL_BG, 2);
    lcd_draw_text(6, 47, calculator_widget_tail(history_line, 78),
                  COL_TEXT, COL_BG, 1);
    lcd_draw_text(6, 64, calculator_widget_tail(result_line, 78),
                  COL_MUTED, COL_BG, 1);
    finish_display();
}

void calculator_page_render_symbols(const calculator_symbols_t *symbols,
                                    size_t selected_function,
                                    const char *message) {
    char status[64];
    char variables[2][80];
    char function[CALCULATOR_USER_FUNCTION_COUNT][79];

    snprintf(status, sizeof status, "SYMBOLS  F%u  %s",
             (unsigned int)(selected_function + 1), message);
    snprintf(variables[0], sizeof variables[0], "A %.5g   B %.5g   C %.5g",
             symbols->variables[0], symbols->variables[1],
             symbols->variables[2]);
    snprintf(variables[1], sizeof variables[1], "D %.5g   E %.5g   F %.5g",
             symbols->variables[3], symbols->variables[4],
             symbols->variables[5]);
    for (size_t i = 0; i < CALCULATOR_USER_FUNCTION_COUNT; ++i) {
        snprintf(function[i], sizeof function[i], "F%u(x)=%.71s",
                 (unsigned int)(i + 1),
                 symbols->functions[i][0] ? symbols->functions[i] : "<empty>");
    }

    clear_display();
    lcd_draw_text(6, 3, status, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 16, variables[0], COL_TEXT, COL_BG, 1);
    lcd_draw_text(6, 29, variables[1], COL_TEXT, COL_BG, 1);
    for (size_t i = 0; i < CALCULATOR_USER_FUNCTION_COUNT; ++i) {
        lcd_draw_text(6, 42 + (int)i * 13, function[i],
                      i == selected_function ? COL_TEXT : COL_MUTED,
                      COL_BG, 1);
    }
    finish_display();
}

static const char *logic_view_name(calculator_logic_view_t view) {
    switch (view) {
        case LOGIC_VIEW_TABLE: return "TABLE";
        case LOGIC_VIEW_DNF: return "DNF";
        case LOGIC_VIEW_KNF: return "KNF";
        case LOGIC_VIEW_GATES: return "GATES";
        case LOGIC_VIEW_EDIT:
        default: return "EDIT";
    }
}

static void format_logic_output(const calculator_logic_t *logic,
                                char *output, size_t output_size) {
    size_t length = 0;
    int written = snprintf(output, output_size, "OUT");
    if (written < 0 || (size_t)written >= output_size) return;
    length = (size_t)written;
    if (logic->program.variable_mask) {
        written = snprintf(output + length, output_size - length, "(");
        if (written < 0 || (size_t)written >= output_size - length) return;
        length += (size_t)written;
        bool first = true;
        for (unsigned int variable = 0; variable < LOGIC_VARIABLE_COUNT;
             ++variable) {
            uint8_t bit = (uint8_t)(1u << variable);
            if (!(logic->program.variable_mask & bit)) continue;
            written = snprintf(output + length, output_size - length,
                               "%s%c=%u", first ? "" : " ",
                               (char)('A' + variable),
                               (logic->assignment & bit) ? 1u : 0u);
            if (written < 0 || (size_t)written >= output_size - length) return;
            length += (size_t)written;
            first = false;
        }
        written = snprintf(output + length, output_size - length, ")");
        if (written < 0 || (size_t)written >= output_size - length) return;
        length += (size_t)written;
    }
    snprintf(output + length, output_size - length, " = %u",
             logic_engine_evaluate(&logic->program, logic->assignment)
                ? 1u : 0u);
}

static void append_char(char *buffer, size_t capacity, size_t *length,
                        char value) {
    if (*length + 1 >= capacity) return;
    buffer[(*length)++] = value;
    buffer[*length] = '\0';
}

static void render_logic_table(const calculator_logic_t *logic) {
    char header[32] = "ROW ";
    size_t header_length = strlen(header);
    for (unsigned int variable = 0; variable < LOGIC_VARIABLE_COUNT;
         ++variable) {
        if (!(logic->program.variable_mask & (1u << variable))) continue;
        append_char(header, sizeof header, &header_length,
                    (char)('A' + variable));
        append_char(header, sizeof header, &header_length, ' ');
    }
    snprintf(header + header_length, sizeof header - header_length, "| Y");
    bool fullscreen = calculator_widget_fullscreen();
    if (fullscreen) {
        page_draw_text_absolute(6, 22, header, COL_TEXT, COL_BG, 2);
    } else {
        lcd_draw_text(6, 17, header, COL_TEXT, COL_BG, 1);
    }

    size_t rows = logic_engine_truth_row_count(&logic->program);
    size_t visible_lines = fullscreen ? 16u : 4u;
    for (size_t line = 0;
         line < visible_lines && logic->scroll + line < rows; ++line) {
        size_t row = logic->scroll + line;
        uint8_t assignment = logic_engine_assignment_for_row(&logic->program,
                                                              row);
        char text[32];
        int used = snprintf(text, sizeof text, "%3u ", (unsigned int)row);
        size_t length = used > 0 ? (size_t)used : 0;
        for (unsigned int variable = 0; variable < LOGIC_VARIABLE_COUNT;
             ++variable) {
            if (!(logic->program.variable_mask & (1u << variable))) continue;
            append_char(text, sizeof text, &length,
                        (assignment & (1u << variable)) ? '1' : '0');
            append_char(text, sizeof text, &length, ' ');
        }
        snprintf(text + length, sizeof text - length, "| %u",
                 logic_engine_evaluate(&logic->program, assignment) ? 1u : 0u);
        if (fullscreen) {
            page_draw_text_absolute(6, 46 + (int)line * 17, text,
                                    COL_MUTED, COL_BG, 2);
        } else {
            lcd_draw_text(6, 30 + (int)line * 13, text,
                          COL_MUTED, COL_BG, 1);
        }
    }
}

static void render_logic_form(const calculator_logic_t *logic) {
    size_t length = strlen(logic->form);
    size_t offset = logic->scroll < length ? logic->scroll : length;
    bool fullscreen = calculator_widget_fullscreen();
    size_t visible_lines = fullscreen ? 16u : 5u;
    size_t line_capacity = fullscreen ? 39u : 78u;
    for (size_t line = 0; line < visible_lines && offset < length; ++line) {
        char text[79];
        size_t remaining = length - offset;
        size_t count = remaining < line_capacity ? remaining : line_capacity;
        memcpy(text, logic->form + offset, count);
        text[count] = '\0';
        if (fullscreen) {
            page_draw_text_absolute(6, 22 + (int)line * 18, text,
                                    line ? COL_MUTED : COL_TEXT,
                                    COL_BG, 2);
        } else {
            lcd_draw_text(6, 17 + (int)line * 13, text,
                          line ? COL_MUTED : COL_TEXT, COL_BG, 1);
        }
        offset += count;
    }
}

static void render_logic_gates(const calculator_logic_t *logic) {
    char inputs[48] = "INPUT ";
    size_t input_length = strlen(inputs);
    unsigned int counts[7] = {0};
    for (unsigned int variable = 0; variable < LOGIC_VARIABLE_COUNT;
         ++variable) {
        if (!(logic->program.variable_mask & (1u << variable))) continue;
        int used = snprintf(inputs + input_length,
                            sizeof inputs - input_length, "%c=%u ",
                            (char)('A' + variable),
                            (logic->assignment & (1u << variable)) ? 1u : 0u);
        if (used > 0) input_length += (size_t)used;
    }
    for (size_t i = 0; i < logic->program.node_count; ++i) {
        logic_node_kind_t kind = logic->program.nodes[i].kind;
        if (kind >= LOGIC_NODE_NOT && kind <= LOGIC_NODE_XNOR) {
            counts[kind - LOGIC_NODE_NOT]++;
        }
    }
    char gates[78];
    snprintf(gates, sizeof gates,
             "NOT%u AND%u OR%u XOR%u NAND%u NOR%u XNOR%u",
             counts[0], counts[1], counts[2], counts[3],
             counts[4], counts[5], counts[6]);
    char output[16];
    snprintf(output, sizeof output, "OUT = %u",
             logic_engine_evaluate(&logic->program, logic->assignment)
                ? 1u : 0u);

    lcd_draw_text(6, 17, calculator_widget_tail(logic->editor.text, 78),
                  COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 32, inputs, COL_TEXT, COL_BG, 1);
    lcd_draw_text(6, 47, output, COL_TEXT, COL_BG, 2);
    lcd_draw_text(6, 70, gates, COL_MUTED, COL_BG, 1);
}

void calculator_page_render_logic(const calculator_logic_t *logic,
                                  const char *message) {
    char status[79];
    char editor_text[EXPRESSION_EDITOR_CAPACITY + 2];
    snprintf(status, sizeof status, "LOGIC %s  %.60s",
             logic_view_name(logic->view), message);
    clear_display();
    lcd_draw_text(6, 3, status, COL_MUTED, COL_BG, 1);

    if (logic->view == LOGIC_VIEW_TABLE && logic->compiled) {
        render_logic_table(logic);
    } else if ((logic->view == LOGIC_VIEW_DNF ||
                logic->view == LOGIC_VIEW_KNF) && logic->compiled) {
        render_logic_form(logic);
    } else if (logic->view == LOGIC_VIEW_GATES && logic->compiled) {
        render_logic_gates(logic);
    } else {
        lcd_draw_text(6, 20,
                      expression_editor_view(&logic->editor, editor_text,
                                             sizeof editor_text, 38),
                      COL_TEXT, COL_BG, 2);
        if (logic->compiled) {
            char output[48];
            format_logic_output(logic, output, sizeof output);
            lcd_draw_text(6, 47, output, COL_TEXT, COL_BG, 2);
            lcd_draw_text(6, 69, "GATES CHANGES INPUTS A-F",
                          COL_MUTED, COL_BG, 1);
        } else {
            lcd_draw_text(6, 52,
                          "A-F  NOT AND OR XOR NAND NOR XNOR",
                          COL_MUTED, COL_BG, 1);
            lcd_draw_text(6, 67,
                          "CHECK OR SELECT TABLE / DNF / KNF / GATES",
                          COL_MUTED, COL_BG, 1);
        }
    }
    finish_display();
}

void calculator_page_render_units(const calculator_units_t *units,
                                  const char *message) {
    char status[79];
    clear_display();
    if (units->view == UNITS_VIEW_CONSTANTS) {
        const physical_constant_t *constant =
            unit_engine_constant(units->constant_index);
        size_t count = unit_engine_constant_count();
        snprintf(status, sizeof status, "CONSTANT %u/%u  %.54s",
                 (unsigned int)(units->constant_index + 1),
                 (unsigned int)count, message);
        lcd_draw_text(6, 3, status, COL_MUTED, COL_BG, 1);
        if (constant) {
            char value[79];
            char unit[79];
            lcd_draw_text(6, 18, constant->name, COL_TEXT, COL_BG, 2);
            snprintf(value, sizeof value, "%s = %.12g",
                     constant->symbol, constant->value);
            snprintf(unit, sizeof unit, "UNIT %s", constant->unit);
            lcd_draw_text(6, 47, value, COL_TEXT, COL_BG, 1);
            lcd_draw_text(6, 61, unit, COL_MUTED, COL_BG, 1);
            lcd_draw_text(6, 72, constant->source, COL_MUTED, COL_BG, 1);
        }
        finish_display();
        return;
    }

    const unit_definition_t *from =
        unit_engine_unit(units->category, units->from_index);
    const unit_definition_t *to =
        unit_engine_unit(units->category, units->to_index);
    char names[79];
    char input[48];
    char output[48];
    snprintf(status, sizeof status, "UNITS %s  %.55s",
             unit_engine_category_name(units->category), message);
    snprintf(names, sizeof names, "%s [%s]  ->  %s [%s]",
             from ? from->name : "?", from ? from->symbol : "?",
             to ? to->name : "?", to ? to->symbol : "?");
    if (units->has_input && from) {
        snprintf(input, sizeof input, "IN  %.12g %s",
                 units->input, from->symbol);
    } else {
        snprintf(input, sizeof input, "IN  --");
    }
    if (units->has_result && to) {
        snprintf(output, sizeof output, "OUT %.12g %s",
                 units->result, to->symbol);
    } else {
        snprintf(output, sizeof output, "OUT --");
    }

    lcd_draw_text(6, 3, status, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 18, names, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 34, calculator_widget_tail(input, 38),
                  COL_TEXT, COL_BG, 2);
    lcd_draw_text(6, 59, calculator_widget_tail(output, 38),
                  COL_TEXT, COL_BG, 2);
    finish_display();
}

void calculator_page_render_complex(const calculator_complex_t *complex,
                                    bool degrees, const char *message) {
    char status[79];
    char editor_text[EXPRESSION_EDITOR_CAPACITY + 2];
    char result[64] = "--";
    const char *expression = complex->editor.text;
    complex_value_t shown = complex->result;
    bool has_result = complex->has_result;

    if (complex->history_view && complex->history_count) {
        const calculator_complex_history_entry_t *entry =
            &complex->history[complex->history_index];
        expression = entry->expression;
        shown = entry->result;
        has_result = true;
        snprintf(status, sizeof status, "COMPLEX HISTORY %u/%u  %.42s",
                 (unsigned int)(complex->history_index + 1),
                 (unsigned int)complex->history_count, message);
    } else {
        snprintf(status, sizeof status, "COMPLEX %s %s  %.49s",
                 complex->polar_view ? "POLAR" : "CART",
                 degrees ? "DEG" : "RAD", message);
    }
    if (has_result) {
        complex_engine_format(shown, complex->polar_view, degrees,
                              result, sizeof result);
    }

    clear_display();
    lcd_draw_text(6, 3, status, COL_MUTED, COL_BG, 1);
    if (complex->history_view) {
        lcd_draw_text(6, 20, calculator_widget_tail(expression, 38),
                      COL_TEXT, COL_BG, 2);
    } else {
        expression_editor_t shown_editor = complex->editor;
        lcd_draw_text(6, 20,
                      expression_editor_view(&shown_editor, editor_text,
                                             sizeof editor_text, 38),
                      COL_TEXT, COL_BG, 2);
    }
    lcd_draw_text(6, 53, calculator_widget_tail(result, 38),
                  has_result ? COL_TEXT : COL_MUTED, COL_BG, 2);
    finish_display();
}

static const char *statistics_view_name(calculator_statistics_view_t view) {
    switch (view) {
        case STATISTICS_VIEW_SUMMARY: return "SUMMARY";
        case STATISTICS_VIEW_REGRESSION: return "REGRESSION";
        case STATISTICS_VIEW_PLOT: return "PLOT";
        case STATISTICS_VIEW_DATA:
        default: return "DATA";
    }
}

static void render_statistics_data(const calculator_statistics_t *stats) {
    char x_text[EXPRESSION_EDITOR_CAPACITY + 2];
    char y_text[EXPRESSION_EDITOR_CAPACITY + 2];
    char x_line[42];
    char y_line[42];
    char selected[79];
    snprintf(x_line, sizeof x_line, "X %s",
             expression_editor_view(&stats->x_editor, x_text,
                                    sizeof x_text, 35));
    snprintf(y_line, sizeof y_line, "Y %s",
             expression_editor_view(&stats->y_editor, y_text,
                                    sizeof y_text, 35));
    if (stats->dataset.count) {
        if (stats->dataset.two_variable) {
            snprintf(selected, sizeof selected, "ROW %u/%u  X %.9g  Y %.9g",
                     (unsigned int)(stats->selected + 1),
                     (unsigned int)stats->dataset.count,
                     stats->dataset.x[stats->selected],
                     stats->dataset.y[stats->selected]);
        } else {
            snprintf(selected, sizeof selected, "ROW %u/%u  X %.12g",
                     (unsigned int)(stats->selected + 1),
                     (unsigned int)stats->dataset.count,
                     stats->dataset.x[stats->selected]);
        }
    } else {
        snprintf(selected, sizeof selected, "NO DATA  ENTER VALUE THEN ADD");
    }

    lcd_draw_text(6, 18, x_line,
                  stats->active_y ? COL_MUTED : COL_TEXT, COL_BG, 2);
    if (stats->dataset.two_variable) {
        lcd_draw_text(6, 45, y_line,
                      stats->active_y ? COL_TEXT : COL_MUTED, COL_BG, 2);
        lcd_draw_text(6, 70, selected, COL_MUTED, COL_BG, 1);
    } else {
        lcd_draw_text(6, 51, selected, COL_MUTED, COL_BG, 2);
    }
}

static void render_statistics_summary(const calculator_statistics_t *stats) {
    statistics_summary_t summary;
    statistics_status_t status = statistics_engine_summary(
        &stats->dataset, stats->active_y, &summary);
    if (status != STATISTICS_STATUS_OK) {
        lcd_draw_text(6, 28, statistics_engine_status_text(status),
                      COL_TEXT, COL_BG, 2);
        return;
    }
    char lines[5][79];
    snprintf(lines[0], sizeof lines[0], "%c  N %u  MEAN %.12g",
             stats->active_y ? 'Y' : 'X', (unsigned int)summary.count,
             summary.mean);
    snprintf(lines[1], sizeof lines[1], "MEDIAN %.12g", summary.median);
    snprintf(lines[2], sizeof lines[2], "MIN %.12g  MAX %.12g",
             summary.minimum, summary.maximum);
    snprintf(lines[3], sizeof lines[3], "SIGMA %.12g",
             summary.population_stddev);
    snprintf(lines[4], sizeof lines[4], "SAMPLE S %.12g",
             summary.sample_stddev);
    for (size_t i = 0; i < 5; ++i) {
        lcd_draw_text(6, 17 + (int)i * 13, lines[i],
                      i == 0 ? COL_TEXT : COL_MUTED, COL_BG, 1);
    }
}

static void render_statistics_regression(const calculator_statistics_t *stats) {
    statistics_regression_t regression;
    statistics_status_t status = statistics_engine_regression(
        &stats->dataset, &regression);
    if (status != STATISTICS_STATUS_OK) {
        lcd_draw_text(6, 28, statistics_engine_status_text(status),
                      COL_TEXT, COL_BG, 2);
        return;
    }
    char equation[64];
    char correlation[64];
    snprintf(equation, sizeof equation, "Y = %.8g X %+.8g",
             regression.slope, regression.intercept);
    snprintf(correlation, sizeof correlation, "r = %.10g   r2 = %.10g",
             regression.correlation,
             regression.correlation * regression.correlation);
    lcd_draw_text(6, 22, calculator_widget_tail(equation, 38),
                  COL_TEXT, COL_BG, 2);
    lcd_draw_text(6, 54, calculator_widget_tail(correlation, 38),
                  COL_MUTED, COL_BG, 2);
}

static int plot_coordinate(double value, double minimum, double maximum,
                           int low, int high) {
    if (maximum == minimum) return (low + high) / 2;
    double ratio = (value - minimum) / (maximum - minimum);
    if (ratio < 0.0) ratio = 0.0;
    if (ratio > 1.0) ratio = 1.0;
    return low + (int)(ratio * (double)(high - low));
}

static void render_statistics_histogram(const calculator_statistics_t *stats) {
    size_t counts[STATISTICS_HISTOGRAM_BINS];
    double minimum;
    double maximum;
    if (statistics_engine_histogram(&stats->dataset, counts,
                                    &minimum, &maximum) != STATISTICS_STATUS_OK) {
        lcd_draw_text(6, 28, "NO DATA", COL_TEXT, COL_BG, 2);
        return;
    }
    size_t largest = 1;
    for (size_t i = 0; i < STATISTICS_HISTOGRAM_BINS; ++i) {
        if (counts[i] > largest) largest = counts[i];
    }
    const int left = 30;
    const int top = display_y(16);
    const int bottom = display_y(79);
    const int width = 446;
    const int bar_width = width / (int)STATISTICS_HISTOGRAM_BINS;
    const int plot_height = bottom - top - 7;
    lcd_fill_rect(left, top, 1, bottom - top + 1, COL_MUTED);
    lcd_fill_rect(left, bottom, width, 1, COL_MUTED);
    for (size_t i = 0; i < STATISTICS_HISTOGRAM_BINS; ++i) {
        int height = (int)(counts[i] * (size_t)plot_height / largest);
        if (!height) continue;
        int x = left + (int)i * bar_width + 3;
        lcd_fill_rect(x, bottom - height, bar_width - 6, height, COL_TEXT);
    }
}

static void render_statistics_scatter(const calculator_statistics_t *stats) {
    statistics_summary_t x_summary;
    statistics_summary_t y_summary;
    if (statistics_engine_summary(&stats->dataset, false, &x_summary) !=
            STATISTICS_STATUS_OK ||
        statistics_engine_summary(&stats->dataset, true, &y_summary) !=
            STATISTICS_STATUS_OK) {
        lcd_draw_text(6, 28, "NO DATA", COL_TEXT, COL_BG, 2);
        return;
    }
    double x_min = x_summary.minimum;
    double x_max = x_summary.maximum;
    double y_min = y_summary.minimum;
    double y_max = y_summary.maximum;
    if (x_min == x_max) { x_min -= 1.0; x_max += 1.0; }
    if (y_min == y_max) { y_min -= 1.0; y_max += 1.0; }
    const int left = 30;
    const int right = 476;
    const int top = display_y(16);
    const int bottom = display_y(80);
    lcd_fill_rect(left, top, 1, bottom - top + 1, COL_MUTED);
    lcd_fill_rect(left, bottom, right - left, 1, COL_MUTED);
    for (size_t i = 0; i < stats->dataset.count; ++i) {
        int x = plot_coordinate(stats->dataset.x[i], x_min, x_max,
                                left + 2, right - 3);
        int y = plot_coordinate(stats->dataset.y[i], y_min, y_max,
                                bottom - 2, top + 2);
        lcd_fill_rect(x - 1, y - 1, 3, 3, COL_TEXT);
    }
    statistics_regression_t regression;
    if (statistics_engine_regression(&stats->dataset, &regression) ==
        STATISTICS_STATUS_OK) {
        for (int step = 0; step <= 48; ++step) {
            double x_value = x_min + (x_max - x_min) * step / 48.0;
            double y_value = regression.slope * x_value +
                             regression.intercept;
            int x = plot_coordinate(x_value, x_min, x_max,
                                    left + 2, right - 3);
            int y = plot_coordinate(y_value, y_min, y_max,
                                    bottom - 2, top + 2);
            lcd_fill_rect(x, y, 2, 2, COL_MUTED);
        }
    }
}

void calculator_page_render_statistics(const calculator_statistics_t *stats,
                                       const char *message) {
    char status[79];
    snprintf(status, sizeof status, "STATS %s %s N%u %c  %.45s",
             stats->dataset.two_variable ? "2VAR" : "1VAR",
             statistics_view_name(stats->view),
             (unsigned int)stats->dataset.count,
             stats->active_y ? 'Y' : 'X', message);
    clear_display();
    lcd_draw_text(6, 3, status, COL_MUTED, COL_BG, 1);
    switch (stats->view) {
        case STATISTICS_VIEW_SUMMARY:
            render_statistics_summary(stats);
            break;
        case STATISTICS_VIEW_REGRESSION:
            render_statistics_regression(stats);
            break;
        case STATISTICS_VIEW_PLOT:
            if (stats->dataset.two_variable) {
                render_statistics_scatter(stats);
            } else {
                render_statistics_histogram(stats);
            }
            break;
        case STATISTICS_VIEW_DATA:
        default:
            render_statistics_data(stats);
            break;
    }
    finish_display();
}
