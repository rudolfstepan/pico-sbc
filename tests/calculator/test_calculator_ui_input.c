#include "calculator_ui_input.h"

#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    calculator_touch_input_t input;
    calculator_touch_input_init(&input);
    int first;
    int second;
    const void *target = NULL;
    CHECK(calculator_touch_input_update(&input, true, 10, 10, &first,
                                        &target) == CALCULATOR_TOUCH_PRESSED);
    CHECK(target == &first);
    CHECK(calculator_touch_input_update(&input, true, 11, 10, &first,
                                        &target) == CALCULATOR_TOUCH_NONE);
    CHECK(calculator_touch_input_update(&input, false, 0, 0, NULL,
                                        &target) == CALCULATOR_TOUCH_ACTIVATED);
    CHECK(target == &first && input.last_x == 11u);

    CHECK(calculator_touch_input_update(&input, true, 10, 10, &first,
                                        &target) == CALCULATOR_TOUCH_PRESSED);
    CHECK(calculator_touch_input_update(&input, true, 30, 10, &second,
                                        &target) == CALCULATOR_TOUCH_CANCELLED);
    CHECK(target == &first);
    CHECK(calculator_touch_input_update(&input, false, 0, 0, NULL,
                                        &target) == CALCULATOR_TOUCH_NONE);
    puts("calculator UI input tests passed");
    return 0;
}
