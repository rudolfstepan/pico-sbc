#include "calculation_status.h"

#include <stdio.h>
#include <string.h>

static int failures;

static void expect_text(calc_status_t status, const char *expected) {
    const char *actual = calculation_status_text(status);
    if (strcmp(actual, expected) != 0) {
        printf("FAIL: status %d -> %s, expected %s\n",
               status, actual, expected);
        failures++;
    }
}

int main(void) {
    expect_text(CALC_OK, "OK");
    expect_text(CALC_PARSE_ERROR, "SYNTAX ERROR");
    expect_text(CALC_DOMAIN_ERROR, "MATH ERROR");
    expect_text(CALC_RANGE_ERROR, "RANGE ERROR");
    expect_text(CALC_CONVERGENCE_ERROR, "NO CONVERGENCE");
    expect_text((calc_status_t)99, "ERROR");

    if (CALC_OVERFLOW != CALC_RANGE_ERROR) {
        printf("FAIL: overflow compatibility alias\n");
        failures++;
    }
    if (failures) return 1;
    printf("calculation status tests passed\n");
    return 0;
}
