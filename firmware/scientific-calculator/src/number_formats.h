#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    NUMBER_FORMAT_IEEE_ZERO,
    NUMBER_FORMAT_IEEE_SUBNORMAL,
    NUMBER_FORMAT_IEEE_NORMAL,
    NUMBER_FORMAT_IEEE_INFINITY,
    NUMBER_FORMAT_IEEE_NAN
} number_format_ieee_class_t;

typedef struct {
    bool sign;
    unsigned int exponent_bits;
    unsigned int mantissa_bits;
    uint16_t raw_exponent;
    int unbiased_exponent;
    uint64_t mantissa;
    number_format_ieee_class_t classification;
} number_format_ieee_t;

uint64_t number_format_mask(unsigned int bits);
uint64_t number_format_apply_width(uint64_t value, unsigned int bits);
uint64_t number_format_twos_negate(uint64_t value, unsigned int bits);
uint64_t number_format_sign_extend(uint64_t value, unsigned int bits);
double number_format_fixed_value(uint64_t value, unsigned int bits,
                                 unsigned int fractional_bits);
void number_format_signed_text(uint64_t value, unsigned int bits,
                               char *buffer, size_t buffer_size);
uint32_t number_format_float32_bits(double value);
uint64_t number_format_float64_bits(double value);
double number_format_bits_float32(uint32_t bits);
double number_format_bits_float64(uint64_t bits);
uint64_t number_format_rotate_left(uint64_t value, unsigned int bits,
                                   unsigned int amount);
uint64_t number_format_rotate_right(uint64_t value, unsigned int bits,
                                    unsigned int amount);
uint64_t number_format_swap_endian(uint64_t value, unsigned int bits);
uint64_t number_format_set_bit(uint64_t value, unsigned int bits,
                               unsigned int index);
uint64_t number_format_clear_bit(uint64_t value, unsigned int bits,
                                 unsigned int index);
uint64_t number_format_toggle_bit(uint64_t value, unsigned int bits,
                                  unsigned int index);
bool number_format_test_bit(uint64_t value, unsigned int bits,
                            unsigned int index);
number_format_ieee_t number_format_inspect_float32(uint32_t bits);
number_format_ieee_t number_format_inspect_float64(uint64_t bits);
const char *number_format_ieee_class_name(number_format_ieee_class_t value);
