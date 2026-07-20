#include "calculator_keymaps.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int check_page(calc_page_t page, size_t expected_count,
                      uint8_t columns, uint8_t rows) {
    size_t count = 0;
    const calc_key_t *keys = calculator_keymap(page, &count);
    CHECK(keys != NULL);
    CHECK(count == expected_count);

    for (size_t i = 0; i < count; ++i) {
        CHECK(keys[i].label != NULL);
        CHECK(keys[i].token != NULL);
        CHECK(keys[i].col < columns);
        CHECK(keys[i].row < rows);
        for (size_t j = i + 1; j < count; ++j) {
            CHECK(keys[i].col != keys[j].col || keys[i].row != keys[j].row);
        }
    }
    return 0;
}

int main(void) {
    CHECK(check_page(PAGE_BASIC, 30, 5, 6) == 0);
    CHECK(check_page(PAGE_SCIENTIFIC, 30, 5, 6) == 0);
    CHECK(check_page(PAGE_PROGRAMMER, 30, 6, 5) == 0);
    CHECK(check_page(PAGE_FORMAT, 30, 6, 5) == 0);
    CHECK(check_page(PAGE_TOOLS, 30, 6, 5) == 0);
    size_t tools_count = 0;
    const calc_key_t *tools = calculator_keymap(PAGE_TOOLS, &tools_count);
    bool found_precision = false;
    bool found_visible_x = false;
    for (size_t i = 0; i < tools_count; ++i) {
        if (tools[i].action == ACT_PRECISION) found_precision = true;
        if (strcmp(tools[i].token, "x") == 0 && tools[i].row == 4u) {
            found_visible_x = true;
        }
    }
    CHECK(found_precision && found_visible_x);
    CHECK(check_page(PAGE_SYMBOLS, 30, 6, 5) == 0);
    CHECK(check_page(PAGE_GRAPH, 6, 6, 5) == 0);
    CHECK(check_page(PAGE_LOGIC, 30, 6, 5) == 0);
    CHECK(check_page(PAGE_UNITS, 30, 6, 5) == 0);
    CHECK(check_page(PAGE_COMPLEX, 30, 6, 5) == 0);
    CHECK(check_page(PAGE_STATISTICS, 30, 6, 5) == 0);
    CHECK(check_page(PAGE_LAUNCHER, 14, 3, 5) == 0);
    CHECK(check_page(PAGE_SETTINGS, 12, 3, 4) == 0);
    CHECK(check_page(PAGE_NUMBER_THEORY, 30, 6, 5) == 0);
    size_t circuit_count = 1u;
    CHECK(calculator_keymap(PAGE_CIRCUIT, &circuit_count) == NULL);
    CHECK(circuit_count == 0u);
    size_t unit_input_count = 0;
    const calc_key_t *unit_input = calculator_units_keymap(
        UNITS_VIEW_CONVERTER, UNITS_SELECTOR_NONE, &unit_input_count);
    bool unit_digits[10] = {false};
    bool found_unit_decimal = false;
    bool found_unit_exponent = false;
    for (size_t i = 0; i < unit_input_count; ++i) {
        if (unit_input[i].token[0] >= '0' && unit_input[i].token[0] <= '9' &&
            unit_input[i].token[1] == '\0') {
            unit_digits[unit_input[i].token[0] - '0'] = true;
        }
        if (unit_input[i].token[0] == '.' && unit_input[i].token[1] == '\0') {
            found_unit_decimal = true;
        }
        if (strcmp(unit_input[i].token, "EXP") == 0) {
            found_unit_exponent = true;
        }
    }
    for (size_t digit = 0; digit < 10u; ++digit) CHECK(unit_digits[digit]);
    CHECK(found_unit_decimal && found_unit_exponent);

    size_t unit_selection_count = 0;
    const calc_key_t *unit_selection = calculator_units_keymap(
        UNITS_VIEW_CONVERTER, UNITS_SELECTOR_FROM, &unit_selection_count);
    CHECK(unit_selection != unit_input && unit_selection_count == 30);
    for (size_t choice = 0; choice < 6u; ++choice) {
        CHECK(unit_selection[24u + choice].token[0] == 'U');
    }
    size_t complex_history_count = 0;
    const calc_key_t *complex_history =
        calculator_complex_keymap(true, &complex_history_count);
    CHECK(complex_history != NULL);
    CHECK(complex_history_count == 30);
    for (size_t i = 0; i < complex_history_count; ++i) {
        CHECK(complex_history[i].col == i % 6);
        CHECK(complex_history[i].row == i / 6);
    }
    for (calculator_format_view_t view = FORMAT_VIEW_CONVERSIONS;
         view <= FORMAT_VIEW_IEEE64;
         view = (calculator_format_view_t)(view + 1)) {
        size_t count = 0;
        const calc_key_t *keys = calculator_format_keymap(view, &count);
        CHECK(keys != NULL);
        CHECK(count == 30);
    }
    bool found_graph_edit = false;
    for (graph_view_t view = GRAPH_VIEW_PLOT; view <= GRAPH_VIEW_RANGE;
         view = (graph_view_t)(view + 1)) {
        size_t count = 0;
        const calc_key_t *keys = calculator_graph_keymap(view, &count);
        CHECK(keys != NULL);
        CHECK(count == 6);
        for (size_t i = 0; i < count; ++i) {
            CHECK(keys[i].row == 4);
            CHECK(keys[i].col == i);
            if (view == GRAPH_VIEW_MENU &&
                strcmp(keys[i].token, "EDIT") == 0) {
                found_graph_edit = true;
            }
        }
    }
    CHECK(found_graph_edit);
    puts("keymap tests passed");
    return 0;
}
