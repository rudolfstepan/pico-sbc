#include "calculator_number_theory.h"
#include "number_theory.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    CHECK(number_theory_gcd(48u, 18u) == 6u);
    uint64_t value = 0;
    CHECK(number_theory_lcm(21u, 6u, &value) && value == 42u);
    CHECK(!number_theory_lcm(UINT64_MAX, 2u, &value));
    CHECK(number_theory_is_prime(2u));
    CHECK(number_theory_is_prime(97u));
    CHECK(number_theory_is_prime(UINT64_C(18446744073709551557)));
    CHECK(!number_theory_is_prime(1u));
    CHECK(!number_theory_is_prime(UINT64_MAX));
    CHECK(number_theory_next_prime(100u, &value) && value == 101u);
    CHECK(number_theory_previous_prime(100u, &value) && value == 97u);
    CHECK(number_theory_mod_pow(4u, 13u, 497u) == 445u);
    CHECK(number_theory_totient(36u, &value) && value == 12u);

    uint64_t factors[8];
    bool complete = false;
    size_t count = number_theory_factor(UINT64_C(600851475143), factors,
                                        8u, &complete);
    CHECK(complete && count == 4u);
    CHECK(factors[0] == 71u && factors[1] == 839u &&
          factors[2] == 1471u && factors[3] == 6857u);

    calculator_number_theory_t tool;
    char message[32];
    calculator_number_theory_init(&tool);
    calculator_number_theory_activate(&tool, "A", "0", &value,
                                       message, sizeof message);
    calculator_number_theory_activate(&tool, "4", "0", &value,
                                       message, sizeof message);
    calculator_number_theory_activate(&tool, "8", "0", &value,
                                       message, sizeof message);
    calculator_number_theory_activate(&tool, "B", "0", &value,
                                       message, sizeof message);
    calculator_number_theory_activate(&tool, "1", "0", &value,
                                       message, sizeof message);
    calculator_number_theory_activate(&tool, "8", "0", &value,
                                       message, sizeof message);
    calculator_number_theory_activate(&tool, "GCD", "0", &value,
                                       message, sizeof message);
    CHECK(tool.has_result && tool.last_result == 6u);
    CHECK(strcmp(tool.operation, "ggT / GCD") == 0);
    puts("number theory tests passed");
    return 0;
}
