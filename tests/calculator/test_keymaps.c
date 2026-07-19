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
    CHECK(check_page(PAGE_TOOLS, 18) == 0);
    CHECK(check_page(PAGE_GRAPH, 6) == 0);
    puts("keymap tests passed");
    return 0;
}
