#include "number_formats.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

static void expect_u64(const char *name, uint64_t actual, uint64_t expected) {
    if (actual != expected) {
        printf("FAIL: %s\n", name);
        failures++;
    }
}

static void expect_double(const char *name, double actual, double expected) {
    if (fabs(actual - expected) > 1e-9) {
        printf("FAIL: %s -> %.12g expected %.12g\n", name, actual, expected);
        failures++;
    }
}

int main(void) {
    char text[24];
    expect_u64("8-bit mask", number_format_apply_width(0x1234, 8), 0x34);
    expect_u64("8-bit negate", number_format_twos_negate(1, 8), 0xff);
    expect_u64("8-bit sign extend", number_format_sign_extend(0x80, 8),
               UINT64_C(0xffffffffffffff80));
    number_format_signed_text(0xff, 8, text, sizeof text);
    if (strcmp(text, "-1") != 0) failures++;

    expect_double("Q8 fixed positive",
                  number_format_fixed_value(0x0180, 16, 8), 1.5);
    expect_double("Q8 fixed negative",
                  number_format_fixed_value(0xff80, 16, 8), -0.5);

    expect_u64("float32 encode", number_format_float32_bits(1.0), 0x3f800000u);
    expect_u64("float64 encode", number_format_float64_bits(1.0),
               UINT64_C(0x3ff0000000000000));
    expect_double("float32 decode", number_format_bits_float32(0x3f800000u), 1.0);
    expect_double("float64 decode",
                  number_format_bits_float64(UINT64_C(0x3ff0000000000000)), 1.0);

    if (failures) {
        printf("%d number format test(s) failed\n", failures);
        return 1;
    }
    printf("number format tests passed\n");
    return 0;
}
