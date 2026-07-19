#include "calculator_complex.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static void press(calculator_complex_t *complex, const char *token,
                  char *message) {
    calculator_complex_activate(complex, token, true, message, 32);
}

int main(void) {
    calculator_complex_t complex;
    char message[32];
    calculator_complex_init(&complex);
    press(&complex, "3", message);
    press(&complex, "+", message);
    press(&complex, "4", message);
    press(&complex, "i", message);
    press(&complex, "=", message);
    CHECK(complex.has_result && complex.result.real == 3.0 &&
          complex.result.imag == 4.0);
    CHECK(complex.history_count == 1);

    press(&complex, "*", message);
    press(&complex, "i", message);
    press(&complex, "=", message);
    CHECK(fabs(complex.result.real + 4.0) < 1e-9);
    CHECK(fabs(complex.result.imag - 3.0) < 1e-9);
    CHECK(complex.history_count == 2);

    press(&complex, "HIST", message);
    CHECK(complex.history_view && complex.history_index == 1);
    press(&complex, "PREV", message);
    press(&complex, "USE", message);
    CHECK(!complex.history_view);
    CHECK(strcmp(complex.editor.text, "3+4i") == 0);
    CHECK(complex.result.real == 3.0 && complex.result.imag == 4.0);

    press(&complex, "VIEW", message);
    CHECK(complex.polar_view);
    press(&complex, "HCLR", message);
    CHECK(complex.history_count == 0);

    puts("calculator complex tests passed");
    return 0;
}
