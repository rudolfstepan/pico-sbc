#include "programmer_engine.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static int failures;

static void expect_value(const char *name, uint64_t actual, uint64_t expected) {
    if (actual != expected) {
        printf("FAIL: %s -> %" PRIu64 " expected %" PRIu64 "\n",
               name, actual, expected);
        failures++;
    }
}

static void append_text(programmer_engine_t *engine, const char *text) {
    while (*text) {
        if (programmer_engine_append(engine, *text++) != PROGRAMMER_INPUT_OK) {
            printf("FAIL: append_text rejected input\n");
            failures++;
            return;
        }
    }
}

int main(void) {
    programmer_engine_t engine;
    programmer_engine_init(&engine);
    append_text(&engine, "255");
    expect_value("decimal input", engine.value, 255);

    programmer_engine_set_base(&engine, PROGRAMMER_HEX);
    if (strcmp(engine.input, "FF") != 0) failures++;
    programmer_engine_set_base(&engine, PROGRAMMER_BIN);
    if (strcmp(engine.input, "11111111") != 0) failures++;
    if (programmer_engine_append(&engine, '2') != PROGRAMMER_INPUT_INVALID_DIGIT) {
        failures++;
    }

    programmer_engine_clear(&engine);
    programmer_engine_set_base(&engine, PROGRAMMER_HEX);
    append_text(&engine, "F0");
    programmer_engine_set_binary_op(&engine, PROGRAMMER_OP_AND);
    append_text(&engine, "0F");
    if (!programmer_engine_equals(&engine)) failures++;
    expect_value("bitwise and", engine.value, 0);

    programmer_engine_clear(&engine);
    append_text(&engine, "1");
    programmer_engine_shift_left(&engine);
    expect_value("shift left", engine.value, 2);
    programmer_engine_not(&engine);
    expect_value("not", engine.value, UINT64_MAX - 2);
    programmer_engine_negate(&engine);
    expect_value("twos complement", engine.value, 3);

    programmer_engine_clear(&engine);
    programmer_engine_set_base(&engine, PROGRAMMER_HEX);
    append_text(&engine, "FFFFFFFFFFFFFFFF");
    if (programmer_engine_append(&engine, 'F') != PROGRAMMER_INPUT_OVERFLOW) {
        failures++;
    }

    if (failures) {
        printf("%d programmer engine test(s) failed\n", failures);
        return 1;
    }
    printf("programmer engine tests passed\n");
    return 0;
}
