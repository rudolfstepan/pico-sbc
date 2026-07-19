#include "number_formats.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

uint64_t number_format_mask(unsigned int bits) {
    if (bits >= 64) return UINT64_MAX;
    if (bits == 0) return 0;
    return (UINT64_C(1) << bits) - 1u;
}

uint64_t number_format_apply_width(uint64_t value, unsigned int bits) {
    return value & number_format_mask(bits);
}

uint64_t number_format_twos_negate(uint64_t value, unsigned int bits) {
    return (~value + 1u) & number_format_mask(bits);
}

uint64_t number_format_sign_extend(uint64_t value, unsigned int bits) {
    uint64_t masked = number_format_apply_width(value, bits);
    if (bits == 0 || bits >= 64) return masked;
    uint64_t sign_bit = UINT64_C(1) << (bits - 1u);
    return (masked & sign_bit) ? masked | ~number_format_mask(bits) : masked;
}

double number_format_fixed_value(uint64_t value, unsigned int bits,
                                 unsigned int fractional_bits) {
    uint64_t masked = number_format_apply_width(value, bits);
    bool negative = bits > 0 && (masked & (UINT64_C(1) << (bits - 1u))) != 0;
    double signed_value;
    if (negative) {
        uint64_t magnitude = number_format_twos_negate(masked, bits);
        signed_value = -(double)magnitude;
    } else {
        signed_value = (double)masked;
    }

    while (fractional_bits-- > 0) signed_value *= 0.5;
    return signed_value;
}

void number_format_signed_text(uint64_t value, unsigned int bits,
                               char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    uint64_t masked = number_format_apply_width(value, bits);
    bool negative = bits > 0 && (masked & (UINT64_C(1) << (bits - 1u))) != 0;
    if (negative) {
        uint64_t magnitude = number_format_twos_negate(masked, bits);
        snprintf(buffer, buffer_size, "-%llu", (unsigned long long)magnitude);
    } else {
        snprintf(buffer, buffer_size, "%llu", (unsigned long long)masked);
    }
}

uint32_t number_format_float32_bits(double value) {
    float converted = (float)value;
    uint32_t bits;
    memcpy(&bits, &converted, sizeof bits);
    return bits;
}

uint64_t number_format_float64_bits(double value) {
    uint64_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits;
}

double number_format_bits_float32(uint32_t bits) {
    float value;
    memcpy(&value, &bits, sizeof value);
    return (double)value;
}

double number_format_bits_float64(uint64_t bits) {
    double value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

uint64_t number_format_rotate_left(uint64_t value, unsigned int bits,
                                   unsigned int amount) {
    if (bits == 0 || bits > 64) return 0;
    uint64_t masked = number_format_apply_width(value, bits);
    amount %= bits;
    if (amount == 0) return masked;
    if (bits == 64) return (masked << amount) | (masked >> (64u - amount));
    return ((masked << amount) | (masked >> (bits - amount))) &
           number_format_mask(bits);
}

uint64_t number_format_rotate_right(uint64_t value, unsigned int bits,
                                    unsigned int amount) {
    if (bits == 0 || bits > 64) return 0;
    uint64_t masked = number_format_apply_width(value, bits);
    amount %= bits;
    if (amount == 0) return masked;
    if (bits == 64) return (masked >> amount) | (masked << (64u - amount));
    return ((masked >> amount) | (masked << (bits - amount))) &
           number_format_mask(bits);
}

uint64_t number_format_swap_endian(uint64_t value, unsigned int bits) {
    if (bits == 0 || bits > 64 || bits % 8u != 0) return 0;
    unsigned int bytes = bits / 8u;
    uint64_t result = 0;
    value = number_format_apply_width(value, bits);
    for (unsigned int i = 0; i < bytes; ++i) {
        result = (result << 8) | ((value >> (i * 8u)) & 0xffu);
    }
    return result;
}

uint64_t number_format_set_bit(uint64_t value, unsigned int bits,
                               unsigned int index) {
    if (index >= bits || bits > 64) return number_format_apply_width(value, bits);
    return number_format_apply_width(value | (UINT64_C(1) << index), bits);
}

uint64_t number_format_clear_bit(uint64_t value, unsigned int bits,
                                 unsigned int index) {
    if (index >= bits || bits > 64) return number_format_apply_width(value, bits);
    return number_format_apply_width(value & ~(UINT64_C(1) << index), bits);
}

uint64_t number_format_toggle_bit(uint64_t value, unsigned int bits,
                                  unsigned int index) {
    if (index >= bits || bits > 64) return number_format_apply_width(value, bits);
    return number_format_apply_width(value ^ (UINT64_C(1) << index), bits);
}

bool number_format_test_bit(uint64_t value, unsigned int bits,
                            unsigned int index) {
    return index < bits && bits <= 64 &&
           (value & (UINT64_C(1) << index)) != 0;
}

static number_format_ieee_t inspect_ieee(uint64_t bits,
                                         unsigned int exponent_bits,
                                         unsigned int mantissa_bits,
                                         unsigned int bias) {
    uint64_t exponent_mask = (UINT64_C(1) << exponent_bits) - 1u;
    uint64_t mantissa_mask = (UINT64_C(1) << mantissa_bits) - 1u;
    number_format_ieee_t result = {
        .sign = ((bits >> (exponent_bits + mantissa_bits)) & 1u) != 0,
        .exponent_bits = exponent_bits,
        .mantissa_bits = mantissa_bits,
        .raw_exponent = (uint16_t)((bits >> mantissa_bits) & exponent_mask),
        .mantissa = bits & mantissa_mask,
    };
    if (result.raw_exponent == 0) {
        result.unbiased_exponent = 1 - (int)bias;
        result.classification = result.mantissa
            ? NUMBER_FORMAT_IEEE_SUBNORMAL : NUMBER_FORMAT_IEEE_ZERO;
    } else if (result.raw_exponent == exponent_mask) {
        result.unbiased_exponent = 0;
        result.classification = result.mantissa
            ? NUMBER_FORMAT_IEEE_NAN : NUMBER_FORMAT_IEEE_INFINITY;
    } else {
        result.unbiased_exponent = (int)result.raw_exponent - (int)bias;
        result.classification = NUMBER_FORMAT_IEEE_NORMAL;
    }
    return result;
}

number_format_ieee_t number_format_inspect_float32(uint32_t bits) {
    return inspect_ieee(bits, 8, 23, 127);
}

number_format_ieee_t number_format_inspect_float64(uint64_t bits) {
    return inspect_ieee(bits, 11, 52, 1023);
}

const char *number_format_ieee_class_name(number_format_ieee_class_t value) {
    switch (value) {
        case NUMBER_FORMAT_IEEE_ZERO: return "ZERO";
        case NUMBER_FORMAT_IEEE_SUBNORMAL: return "SUBNORMAL";
        case NUMBER_FORMAT_IEEE_NORMAL: return "NORMAL";
        case NUMBER_FORMAT_IEEE_INFINITY: return "INFINITY";
        case NUMBER_FORMAT_IEEE_NAN: return "NAN";
        default: return "?";
    }
}
