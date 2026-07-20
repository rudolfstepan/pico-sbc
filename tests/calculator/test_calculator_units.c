#include "calculator_units.h"

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

    press(&units, "1", 0.0, &output, message);
    press(&units, "2", 0.0, &output, message);
    press(&units, "3", 0.0, &output, message);
    press(&units, ".", 0.0, &output, message);
    press(&units, "4", 0.0, &output, message);
    CHECK(strcmp(units.input_text, "123.4") == 0);
    CHECK(units.has_input && units.has_result);
    CHECK(fabs(units.input - 123.4) < 1e-12);
    CHECK(fabs(units.result - 0.1234) < 1e-12);

    press(&units, "SIGN", 0.0, &output, message);
    CHECK(strcmp(units.input_text, "-123.4") == 0);
    CHECK(fabs(units.result + 0.1234) < 1e-12);
    press(&units, "SIGN", 0.0, &output, message);
    CHECK(strcmp(units.input_text, "123.4") == 0);

    press(&units, "AC", 0.0, &output, message);
    press(&units, "1", 0.0, &output, message);
    press(&units, "EXP", 0.0, &output, message);
    CHECK(!units.has_input && strcmp(message, "ENTER EXPONENT") == 0);
    press(&units, "SIGN", 0.0, &output, message);
    press(&units, "3", 0.0, &output, message);
    CHECK(strcmp(units.input_text, "1e-3") == 0);
    CHECK(units.has_result && fabs(units.result - 1e-6) < 1e-18);
    press(&units, "DEL", 0.0, &output, message);
    CHECK(!units.has_input && strcmp(units.input_text, "1e-") == 0);

    puts("calculator units tests passed");
    return 0;
}
