#include "calculator_keymaps.h"

#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static int check_page(calc_page_t page, size_t expected_count) {
    size_t count = 0;
    const calc_key_t *keys = calculator_keymap(page, &count);
    CHECK(keys != NULL);
    CHECK(count == expected_count);

    for (size_t i = 0; i < count; ++i) {
        CHECK(keys[i].label != NULL);
        CHECK(keys[i].token != NULL);
        CHECK(keys[i].col < 6);
        CHECK(keys[i].row < 5);
        for (size_t j = i + 1; j < count; ++j) {
            CHECK(keys[i].col != keys[j].col || keys[i].row != keys[j].row);
        }
    }
    return 0;
}

int main(void) {
    CHECK(check_page(PAGE_BASIC, 30) == 0);
    CHECK(check_page(PAGE_SCIENTIFIC, 30) == 0);
    CHECK(check_page(PAGE_PROGRAMMER, 30) == 0);
    CHECK(check_page(PAGE_FORMAT, 30) == 0);
    CHECK(check_page(PAGE_TOOLS, 30) == 0);
    CHECK(check_page(PAGE_SYMBOLS, 30) == 0);
    CHECK(check_page(PAGE_GRAPH, 6) == 0);
    CHECK(check_page(PAGE_LOGIC, 30) == 0);
    CHECK(check_page(PAGE_UNITS, 30) == 0);
    CHECK(check_page(PAGE_COMPLEX, 30) == 0);
    CHECK(check_page(PAGE_STATISTICS, 30) == 0);
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
    for (graph_view_t view = GRAPH_VIEW_PLOT; view <= GRAPH_VIEW_RANGE;
         view = (graph_view_t)(view + 1)) {
        size_t count = 0;
        const calc_key_t *keys = calculator_graph_keymap(view, &count);
        CHECK(keys != NULL);
        CHECK(count == 6);
        for (size_t i = 0; i < count; ++i) {
            CHECK(keys[i].row == 4);
            CHECK(keys[i].col == i);
        }
    }
    puts("keymap tests passed");
    return 0;
}
