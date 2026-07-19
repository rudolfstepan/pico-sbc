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
