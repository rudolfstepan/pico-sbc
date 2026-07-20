#include "number_theory.h"

#include <limits.h>

uint64_t number_theory_gcd(uint64_t a, uint64_t b) {
    while (b) {
        uint64_t remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

bool number_theory_lcm(uint64_t a, uint64_t b, uint64_t *result) {
    if (!result) return false;
    if (!a || !b) {
        *result = 0;
        return true;
    }
    uint64_t divided = a / number_theory_gcd(a, b);
    if (divided > UINT64_MAX / b) return false;
    *result = divided * b;
    return true;
}

static uint64_t mod_add(uint64_t a, uint64_t b, uint64_t modulus) {
    return a >= modulus - b ? a - (modulus - b) : a + b;
}

static uint64_t mod_multiply(uint64_t a, uint64_t b, uint64_t modulus) {
    uint64_t result = 0;
    a %= modulus;
    while (b) {
        if (b & 1u) result = mod_add(result, a, modulus);
        b >>= 1u;
        if (b) a = mod_add(a, a, modulus);
    }
    return result;
}

uint64_t number_theory_mod_pow(uint64_t base, uint64_t exponent,
                               uint64_t modulus) {
    if (!modulus) return 0;
    uint64_t result = 1u % modulus;
    base %= modulus;
    while (exponent) {
        if (exponent & 1u) result = mod_multiply(result, base, modulus);
        exponent >>= 1u;
        if (exponent) base = mod_multiply(base, base, modulus);
    }
    return result;
}

bool number_theory_is_prime(uint64_t value) {
    static const uint32_t small_primes[] = {
        2u, 3u, 5u, 7u, 11u, 13u, 17u, 19u, 23u, 29u, 31u, 37u
    };
    static const uint64_t witnesses[] = {
        UINT64_C(2), UINT64_C(325), UINT64_C(9375), UINT64_C(28178),
        UINT64_C(450775), UINT64_C(9780504), UINT64_C(1795265022)
    };
    if (value < 2u) return false;
    for (size_t i = 0; i < sizeof small_primes / sizeof small_primes[0]; ++i) {
        if (value == small_primes[i]) return true;
        if (value % small_primes[i] == 0u) return false;
    }

    uint64_t odd = value - 1u;
    unsigned int shifts = 0;
    while (!(odd & 1u)) {
        odd >>= 1u;
        shifts++;
    }
    for (size_t i = 0; i < sizeof witnesses / sizeof witnesses[0]; ++i) {
        uint64_t witness = witnesses[i] % value;
        if (!witness) continue;
        uint64_t x = number_theory_mod_pow(witness, odd, value);
        if (x == 1u || x == value - 1u) continue;
        bool probable = false;
        for (unsigned int round = 1; round < shifts; ++round) {
            x = mod_multiply(x, x, value);
            if (x == value - 1u) {
                probable = true;
                break;
            }
        }
        if (!probable) return false;
    }
    return true;
}

bool number_theory_next_prime(uint64_t value, uint64_t *result) {
    if (!result || value >= UINT64_MAX - 1u) return false;
    uint64_t candidate = value < 2u ? 2u : value + 1u;
    if (candidate > 2u && !(candidate & 1u)) candidate++;
    for (;;) {
        if (number_theory_is_prime(candidate)) {
            *result = candidate;
            return true;
        }
        if (candidate > UINT64_MAX - 2u) return false;
        candidate += candidate == 2u ? 1u : 2u;
    }
}

bool number_theory_previous_prime(uint64_t value, uint64_t *result) {
    if (!result || value <= 2u) return false;
    uint64_t candidate = value - 1u;
    if (candidate > 2u && !(candidate & 1u)) candidate--;
    for (;;) {
        if (number_theory_is_prime(candidate)) {
            *result = candidate;
            return true;
        }
        if (candidate <= 3u) return false;
        candidate -= 2u;
    }
}

static uint64_t rho_step(uint64_t value, uint64_t constant,
                         uint64_t modulus) {
    return mod_add(mod_multiply(value, value, modulus), constant, modulus);
}

static uint64_t pollard_rho(uint64_t value) {
    if (!(value & 1u)) return 2u;
    if (value % 3u == 0u) return 3u;
    for (uint64_t constant = 1u; constant <= 24u; ++constant) {
        uint64_t x = 2u;
        uint64_t y = 2u;
        uint64_t divisor = 1u;
        for (unsigned int iteration = 0;
             iteration < 100000u && divisor == 1u; ++iteration) {
            x = rho_step(x, constant, value);
            y = rho_step(rho_step(y, constant, value), constant, value);
            uint64_t difference = x > y ? x - y : y - x;
            divisor = number_theory_gcd(difference, value);
        }
        if (divisor > 1u && divisor < value) return divisor;
    }
    return 0;
}

static void append_factors(uint64_t value, uint64_t *factors,
                           size_t capacity, size_t *count, bool *complete) {
    if (value == 1u) return;
    if (number_theory_is_prime(value)) {
        if (*count < capacity) factors[(*count)++] = value;
        else *complete = false;
        return;
    }
    uint64_t divisor = pollard_rho(value);
    if (!divisor) {
        *complete = false;
        if (*count < capacity) factors[(*count)++] = value;
        return;
    }
    append_factors(divisor, factors, capacity, count, complete);
    append_factors(value / divisor, factors, capacity, count, complete);
}

size_t number_theory_factor(uint64_t value, uint64_t *factors,
                            size_t capacity, bool *complete) {
    if (complete) *complete = true;
    if (!factors || !capacity || value < 2u) return 0;
    bool local_complete = true;
    size_t count = 0;
    append_factors(value, factors, capacity, &count, &local_complete);
    for (size_t i = 1; i < count; ++i) {
        uint64_t item = factors[i];
        size_t position = i;
        while (position && factors[position - 1u] > item) {
            factors[position] = factors[position - 1u];
            position--;
        }
        factors[position] = item;
    }
    if (complete) *complete = local_complete;
    return count;
}

bool number_theory_totient(uint64_t value, uint64_t *result) {
    if (!result || !value) return false;
    uint64_t factors[64];
    bool complete = true;
    size_t count = number_theory_factor(value, factors, 64u, &complete);
    if (!complete || (!count && value > 1u)) return false;
    uint64_t phi = value;
    uint64_t previous = 0;
    for (size_t i = 0; i < count; ++i) {
        if (factors[i] == previous) continue;
        previous = factors[i];
        phi = phi / previous * (previous - 1u);
    }
    *result = phi;
    return true;
}
