#ifndef NUMBER_THEORY_H
#define NUMBER_THEORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint64_t number_theory_gcd(uint64_t a, uint64_t b);
bool number_theory_lcm(uint64_t a, uint64_t b, uint64_t *result);
bool number_theory_is_prime(uint64_t value);
bool number_theory_next_prime(uint64_t value, uint64_t *result);
bool number_theory_previous_prime(uint64_t value, uint64_t *result);
uint64_t number_theory_mod_pow(uint64_t base, uint64_t exponent,
                               uint64_t modulus);
size_t number_theory_factor(uint64_t value, uint64_t *factors,
                            size_t capacity, bool *complete);
bool number_theory_totient(uint64_t value, uint64_t *result);

#endif
