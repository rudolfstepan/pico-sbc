#include "calculator_symbols.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int failures;

int main(void) {
    calculator_symbols_t symbols;
    calculator_symbols_init(&symbols);
    if (strcmp(symbols.favorites[0], "f1(") != 0 ||
        strcmp(symbols.favorites[5], "sqrt(") != 0) {
        printf("FAIL: default favorites\n");
        failures++;
    }
    if (!calculator_symbols_set_variable(&symbols, 0, 42.5) ||
        fabs(symbols.variables[0] - 42.5) > 1e-12 ||
        calculator_symbols_set_variable(&symbols, 6, 1.0)) {
        printf("FAIL: variables\n");
        failures++;
    }

    if (calculator_symbols_set_function(&symbols, 0, "x^2+A") !=
            CALCULATOR_SYMBOL_OK ||
        calculator_symbols_set_function(&symbols, 1, "f1(x)+1") !=
            CALCULATOR_SYMBOL_OK ||
        calculator_symbols_function_dependencies(&symbols, 1) != 1u) {
        printf("FAIL: function dependencies\n");
        failures++;
    }
    if (calculator_symbols_set_function(&symbols, 0, "f1(x)") !=
            CALCULATOR_SYMBOL_RECURSIVE ||
        strcmp(symbols.functions[0], "x^2+A") != 0) {
        printf("FAIL: direct recursion rollback\n");
        failures++;
    }
    if (calculator_symbols_set_function(&symbols, 2, "f2(x)") !=
            CALCULATOR_SYMBOL_OK ||
        calculator_symbols_set_function(&symbols, 0, "f3(x)") !=
            CALCULATOR_SYMBOL_RECURSIVE ||
        strcmp(symbols.functions[0], "x^2+A") != 0) {
        printf("FAIL: indirect recursion rollback\n");
        failures++;
    }
    if (calculator_symbols_set_favorite(&symbols, 4, "log(") !=
            CALCULATOR_SYMBOL_OK ||
        strcmp(symbols.favorites[4], "log(") != 0) {
        printf("FAIL: favorite assignment\n");
        failures++;
    }

    if (failures) {
        printf("%d calculator symbol test(s) failed\n", failures);
        return 1;
    }
    printf("calculator symbol tests passed\n");
    return 0;
}
