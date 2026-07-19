#include "calculator_pages.h"
#include "calculator_symbols.h"
#include "mock_lcd.h"

#include <stdio.h>

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

    puts("calculator page tests passed");
    return 0;
}
