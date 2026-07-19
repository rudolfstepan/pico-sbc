#include "complex_engine.h"

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

static int close_value(complex_value_t actual, double real, double imag) {
    return fabs(actual.real - real) < 1e-9 &&
           fabs(actual.imag - imag) < 1e-9;
}

static complex_status_t evaluate(const char *expression, bool degrees,
                                 complex_value_t *result) {
    int error = 0;
    return complex_engine_evaluate(expression, degrees, result, &error);
}

int main(void) {
    complex_value_t result;
    CHECK(evaluate("3+4i", false, &result) == COMPLEX_STATUS_OK);
    CHECK(close_value(result, 3.0, 4.0));
    CHECK(evaluate("(1+i)(1-i)", false, &result) == COMPLEX_STATUS_OK);
    CHECK(close_value(result, 2.0, 0.0));
    CHECK(evaluate("(3+4i)/(1-2i)", false, &result) == COMPLEX_STATUS_OK);
    CHECK(close_value(result, -1.0, 2.0));
    CHECK(evaluate("i^2", false, &result) == COMPLEX_STATUS_OK);
    CHECK(close_value(result, -1.0, 0.0));

    CHECK(evaluate("conj(3+4i)", false, &result) == COMPLEX_STATUS_OK);
    CHECK(close_value(result, 3.0, -4.0));
    CHECK(evaluate("abs(3+4i)", false, &result) == COMPLEX_STATUS_OK);
    CHECK(close_value(result, 5.0, 0.0));
    CHECK(evaluate("arg(1+i)", true, &result) == COMPLEX_STATUS_OK);
    CHECK(close_value(result, 45.0, 0.0));
    CHECK(evaluate("polar(2,90)", true, &result) == COMPLEX_STATUS_OK);
    CHECK(close_value(result, 0.0, 2.0));
    CHECK(evaluate("sqrt(-4)", false, &result) == COMPLEX_STATUS_OK);
    CHECK(close_value(result, 0.0, 2.0));

    char text[64];
    CHECK(complex_engine_format((complex_value_t){0.0, 0.0}, false, false,
                                text, sizeof text));
    CHECK(strcmp(text, "0+0i") == 0);
    CHECK(complex_engine_format((complex_value_t){3.0, 4.0}, true, true,
                                text, sizeof text));
    CHECK(strstr(text, "5 <") == text && strstr(text, "deg"));

    CHECK(evaluate("1/(i-i)", false, &result) == COMPLEX_STATUS_DIV_ZERO);
    CHECK(evaluate("polar(-1,0)", false, &result) == COMPLEX_STATUS_DOMAIN);
    CHECK(evaluate("arg(0)", false, &result) == COMPLEX_STATUS_DOMAIN);
    int error = 0;
    CHECK(complex_engine_evaluate("3+", false, &result, &error) ==
          COMPLEX_STATUS_SYNTAX);
    CHECK(error > 0);

    puts("complex engine tests passed");
    return 0;
}
