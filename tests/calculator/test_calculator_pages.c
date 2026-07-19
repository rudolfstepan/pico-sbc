#include "calculator_pages.h"
#include "calculator_symbols.h"
#include "calculator_widgets.h"
#include "mock_lcd.h"

#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    calculator_symbols_t symbols;
    calculator_symbols_init(&symbols);
    calculator_symbols_set_variable(&symbols, 0, 123456.0);
    calculator_symbols_set_variable(&symbols, 5, -0.000012345);
    calculator_symbols_set_function(
        &symbols, 0,
        "sin(x)+cos(x)+A+B+C+D+E+F+sqrt(abs(x))+x^2+x^3+x^4+x^5");

    mock_lcd_reset();
    calculator_page_render_symbols(&symbols, 0, "F1 SAVED");
    if (mock_lcd_had_out_of_bounds_draw()) {
        puts("FAIL: out-of-bounds drawing on symbols page");
        return 1;
    }

    programmer_engine_t programmer;
    programmer_engine_init(&programmer);
    programmer_engine_set_word_bits(&programmer, 64);
    programmer_engine_set_value(&programmer, UINT64_MAX);
    programmer.signed_mode = true;
    programmer.selected_bit = 63;

    mock_lcd_reset();
    calculator_page_render_programmer(&programmer, "64-BIT READY");
    calculator_page_render_format(&programmer, 24, FORMAT_VIEW_BITS,
                                  "BIT READY");
    calculator_page_render_format(&programmer, 24, FORMAT_VIEW_IEEE32,
                                  "IEEE32 READY");
    calculator_page_render_format(&programmer, 24, FORMAT_VIEW_IEEE64,
                                  "IEEE64 READY");
    if (mock_lcd_had_out_of_bounds_draw()) {
        puts("FAIL: out-of-bounds drawing on programmer format pages");
        return 1;
    }

    calculator_logic_t logic;
    char logic_message[32];
    calculator_logic_init(&logic);
    expression_editor_set(&logic.editor, "1&1");
    calculator_logic_activate(&logic, "CHECK", logic_message,
                              sizeof logic_message);
    mock_lcd_reset();
    calculator_page_render_logic(&logic, logic_message);
    CHECK(mock_lcd_drew_text("OUT = 1"));

    calculator_logic_init(&logic);
    expression_editor_set(&logic.editor, "0|1");
    calculator_logic_activate(&logic, "CHECK", logic_message,
                              sizeof logic_message);
    mock_lcd_reset();
    calculator_page_render_logic(&logic, logic_message);
    CHECK(mock_lcd_drew_text("OUT = 1"));

    calculator_logic_init(&logic);
    expression_editor_set(&logic.editor, "A^B^C^D^E^F");
    calculator_logic_compile(&logic, logic_message, sizeof logic_message);
    calculator_page_render_logic(&logic, logic_message);
    calculator_logic_activate(&logic, "TABLE", logic_message,
                              sizeof logic_message);
    calculator_page_render_logic(&logic, logic_message);
    calculator_logic_activate(&logic, "DNF", logic_message,
                              sizeof logic_message);
    calculator_page_render_logic(&logic, logic_message);
    calculator_logic_activate(&logic, "KNF", logic_message,
                              sizeof logic_message);
    calculator_page_render_logic(&logic, logic_message);
    calculator_logic_activate(&logic, "GATES", logic_message,
                              sizeof logic_message);
    calculator_page_render_logic(&logic, logic_message);
    if (mock_lcd_had_out_of_bounds_draw()) {
        puts("FAIL: out-of-bounds drawing on logic pages");
        return 1;
    }

    calculator_units_t units;
    double units_output = 0.0;
    calculator_units_init(&units);
    calculator_units_activate(&units, "ANSIN", 123456789.0,
                              &units_output, logic_message,
                              sizeof logic_message);
    calculator_page_render_units(&units, logic_message);
    calculator_units_activate(&units, "CONST", 0.0,
                              &units_output, logic_message,
                              sizeof logic_message);
    calculator_page_render_units(&units, logic_message);
    if (mock_lcd_had_out_of_bounds_draw()) {
        puts("FAIL: out-of-bounds drawing on units pages");
        return 1;
    }

    calculator_complex_t complex;
    calculator_complex_init(&complex);
    expression_editor_set(&complex.editor, "polar(123456789,123.456789)");
    calculator_complex_activate(&complex, "=", true,
                                logic_message, sizeof logic_message);
    calculator_page_render_complex(&complex, true, logic_message);
    calculator_complex_activate(&complex, "HIST", true,
                                logic_message, sizeof logic_message);
    calculator_page_render_complex(&complex, true, logic_message);
    if (mock_lcd_had_out_of_bounds_draw()) {
        puts("FAIL: out-of-bounds drawing on complex pages");
        return 1;
    }

    calculator_statistics_t statistics;
    calculator_statistics_init(&statistics);
    for (int value = 0; value < 12; ++value) {
        statistics_engine_add(&statistics.dataset,
                              (double)(value * value), 0.0);
    }
    calculator_page_render_statistics(&statistics, "DATA READY");
    statistics.view = STATISTICS_VIEW_SUMMARY;
    calculator_page_render_statistics(&statistics, "SUMMARY");
    statistics.view = STATISTICS_VIEW_PLOT;
    calculator_page_render_statistics(&statistics, "HISTOGRAM");
    statistics_engine_set_two_variable(&statistics.dataset, true);
    for (int value = 0; value < 12; ++value) {
        statistics_engine_add(&statistics.dataset, (double)value,
                              2.0 * value + 1.0);
    }
    statistics.view = STATISTICS_VIEW_REGRESSION;
    calculator_page_render_statistics(&statistics, "REGRESSION");
    statistics.view = STATISTICS_VIEW_PLOT;
    calculator_page_render_statistics(&statistics, "SCATTER PLOT");
    if (mock_lcd_had_out_of_bounds_draw()) {
        puts("FAIL: out-of-bounds drawing on statistics pages");
        return 1;
    }

    calculator_widget_state_t widget_state = {
        .page = PAGE_STATISTICS,
        .statistics_two_variable = true,
        .statistics_view = STATISTICS_VIEW_PLOT,
    };
    calculator_widget_set_data_focus(true);
    CHECK(calculator_widget_data_focus());
    CHECK(calculator_widget_display_height() == 168);
    CHECK(calculator_widget_key_top(0) == 188);
    CHECK(calculator_widget_key_top(4) == 280);
    CHECK(calculator_widget_key_height() == 21);

    mock_lcd_reset();
    calculator_page_render_programmer(&programmer, "DATA DISPLAY LARGE");
    calculator_page_render_format(&programmer, 24, FORMAT_VIEW_IEEE64,
                                  "DATA DISPLAY LARGE");
    calculator_page_render_symbols(&symbols, 0, "DATA DISPLAY LARGE");
    calculator_page_render_logic(&logic, "DATA DISPLAY LARGE");
    calculator_page_render_units(&units, "DATA DISPLAY LARGE");
    calculator_page_render_complex(&complex, true, "DATA DISPLAY LARGE");
    calculator_page_render_statistics(&statistics, "DATA DISPLAY LARGE");
    calculator_widget_render_keypad(PAGE_STATISTICS, &widget_state);
    CHECK(!mock_lcd_had_out_of_bounds_draw());
    CHECK(mock_lcd_max_text_scale() >= 3);

    const calc_key_t *first = calculator_widget_hit_key(
        PAGE_STATISTICS, &widget_state, 5, 189);
    const calc_key_t *gap = calculator_widget_hit_key(
        PAGE_STATISTICS, &widget_state, 5, 209);
    const calc_key_t *last = calculator_widget_hit_key(
        PAGE_STATISTICS, &widget_state, 5, 300);
    CHECK(first && first->row == 0 && first->col == 0);
    CHECK(gap == NULL);
    CHECK(last && last->row == 4 && last->col == 0);

    calculator_widget_set_data_focus(false);
    CHECK(calculator_widget_display_height() == 84);
    CHECK(calculator_widget_key_top(4) == 272);
    CHECK(calculator_widget_key_height() == 42);

    puts("calculator page tests passed");
    return 0;
}
