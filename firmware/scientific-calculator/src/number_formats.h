#pragma once

#include <stddef.h>
#include <stdint.h>

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
