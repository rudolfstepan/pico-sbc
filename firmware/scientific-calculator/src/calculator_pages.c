#include "calculator_pages.h"

#include "calculator_widgets.h"
#include "lcd_st7796.h"
#include "number_formats.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define COL_BG    RGB565(0, 0, 0)
#define COL_TEXT  RGB565(255, 255, 255)
#define COL_MUTED RGB565(170, 170, 170)

static void clear_display(void) {
    lcd_fill_rect(0, 0, LCD_WIDTH, CALCULATOR_DISPLAY_HEIGHT, COL_BG);
}

static void finish_display(void) {
    lcd_fill_rect(0, CALCULATOR_DISPLAY_HEIGHT - 2, LCD_WIDTH, 2, COL_MUTED);
}

void calculator_page_render_expression(calc_page_t page, bool degrees,
                                       const char *message,
                                       const expression_editor_t *editor,
                                       const char *result_text) {
    char status[48];
    char editor_text[EXPRESSION_EDITOR_CAPACITY + 2];
    char shown_result[36];

    snprintf(status, sizeof status, "%s  %s  %s",
             degrees ? "DEG" : "RAD",
             page == PAGE_SCIENTIFIC ? "SCIENTIFIC" : "BASIC", message);
    snprintf(shown_result, sizeof shown_result, "=%s", result_text);
    const char *visible = calculator_widget_tail(shown_result, 38);
    int width = (int)strlen(visible) * 12;
    int x = LCD_WIDTH - width - 6;
    if (x < 6) x = 6;

    clear_display();
    lcd_draw_text(6, 4, status, COL_MUTED, COL_BG, 1);
    lcd_draw_text(6, 20,
                  expression_editor_view(editor, editor_text,
                                         sizeof editor_text, 38),
                  COL_TEXT, COL_BG, 2);
    lcd_draw_text(x, 52, visible, COL_TEXT, COL_BG, 2);
    finish_display();
}

void calculator_page_render_programmer(const programmer_engine_t *programmer,
                                       const char *message) {
    char status[48];
    char decimal[21];
    char hexadecimal[17];
    char binary[65];
    char conversions[48];
    char binary_line[72];

    programmer_engine_format(programmer->value, PROGRAMMER_DEC,
                             decimal, sizeof decimal);
    programmer_engine_format(programmer->value, PROGRAMMER_HEX,
                             hexadecimal, sizeof hexadecimal);
    programmer_engine_format(programmer->value, PROGRAMMER_BIN,
                             binary, sizeof binary);
    snprintf(status, sizeof status, "PROGRAMMER  %s  %s%s%s",
             programmer_engine_base_name(programmer->base),
             programmer_engine_op_name(programmer->pending_op),
             programmer->pending_op == PROGRAMMER_OP_NONE ? "" : "  ",
             message);
    snprintf(conversions, sizeof conversions, "DEC %s  HEX %s",
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
                                   unsigned int format_bits,
                                   unsigned int fixed_fraction_bits,
                                   const char *message) {
    uint64_t raw = number_format_apply_width(programmer->value, format_bits);
    char hexadecimal[17];
    char signed_text[24];
    char status[48];
    char raw_line[24];
    char fixed_line[52];
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
