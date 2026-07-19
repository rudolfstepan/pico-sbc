#include "calculator_units.h"

#include <math.h>
#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static calculator_units_output_t press(calculator_units_t *units,
                                       const char *token, double ans,
                                       double *output, char *message) {
    return calculator_units_activate(units, token, ans, output,
                                     message, 32);
}

int main(void) {
    calculator_units_t units;
    char message[32];
    double output = 0.0;
    calculator_units_init(&units);
    CHECK(units.category == UNIT_CATEGORY_LENGTH);

    press(&units, "ANSIN", 1609.344, &output, message);
    CHECK(units.has_result && fabs(units.result - 1.609344) < 1e-12);
    CHECK(press(&units, "OUTANS", 0.0, &output, message) ==
          UNITS_OUTPUT_ANS);
    CHECK(fabs(output - 1.609344) < 1e-12);

    press(&units, "TEMPERATURE", 0.0, &output, message);
    press(&units, "ANSIN", 273.15, &output, message);
    CHECK(units.has_result && fabs(units.result) < 1e-12);
    press(&units, "SWAP", 0.0, &output, message);
    CHECK(fabs(units.input) < 1e-12);
    CHECK(fabs(units.result - 273.15) < 1e-12);

    press(&units, "CONST", 0.0, &output, message);
    CHECK(units.view == UNITS_VIEW_CONSTANTS);
    CHECK(press(&units, "CANS", 0.0, &output, message) == UNITS_OUTPUT_ANS);
    CHECK(output == 299792458.0);
    press(&units, "C+", 0.0, &output, message);
    CHECK(press(&units, "CEDIT", 0.0, &output, message) ==
          UNITS_OUTPUT_EDITOR);
    CHECK(output == 6.62607015e-34);

    press(&units, "RESET", 0.0, &output, message);
    CHECK(units.category == UNIT_CATEGORY_LENGTH && !units.has_input);

    puts("calculator units tests passed");
    return 0;
}
