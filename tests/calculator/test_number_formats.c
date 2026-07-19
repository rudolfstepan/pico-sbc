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

    expect_u64("8-bit rotate left",
               number_format_rotate_left(0x81, 8, 1), 0x03);
    expect_u64("8-bit rotate right",
               number_format_rotate_right(0x03, 8, 1), 0x81);
    expect_u64("32-bit endian swap",
               number_format_swap_endian(0x12345678, 32), 0x78563412);
    expect_u64("16-bit endian swap",
               number_format_swap_endian(0x1234, 16), 0x3412);
    expect_u64("64-bit endian swap",
               number_format_swap_endian(UINT64_C(0x0123456789abcdef), 64),
               UINT64_C(0xefcdab8967452301));
    expect_u64("set bit", number_format_set_bit(0, 16, 12), 0x1000);
    expect_u64("clear bit", number_format_clear_bit(0xffff, 16, 12), 0xefff);
    expect_u64("toggle bit", number_format_toggle_bit(0, 8, 7), 0x80);
    if (!number_format_test_bit(0x80, 8, 7) ||
        number_format_test_bit(0x80, 8, 8)) failures++;

    number_format_ieee_t ieee32 = number_format_inspect_float32(0x3f800000u);
    if (ieee32.sign || ieee32.raw_exponent != 127 ||
        ieee32.unbiased_exponent != 0 || ieee32.mantissa != 0 ||
        ieee32.classification != NUMBER_FORMAT_IEEE_NORMAL) failures++;
    number_format_ieee_t ieee64 = number_format_inspect_float64(
        UINT64_C(0xfff0000000000000));
    if (!ieee64.sign || ieee64.raw_exponent != 2047 ||
        ieee64.classification != NUMBER_FORMAT_IEEE_INFINITY) failures++;
    if (number_format_inspect_float32(1).classification !=
            NUMBER_FORMAT_IEEE_SUBNORMAL ||
        number_format_inspect_float32(0x7fc00000u).classification !=
            NUMBER_FORMAT_IEEE_NAN) failures++;

    if (failures) {
        printf("%d number format test(s) failed\n", failures);
        return 1;
    }
    printf("number format tests passed\n");
    return 0;
}
