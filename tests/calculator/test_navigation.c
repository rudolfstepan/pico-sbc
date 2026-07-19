#include "calculator_navigation.h"

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
    CHECK(calculator_navigation_next(PAGE_BASIC) == PAGE_SCIENTIFIC);
    CHECK(calculator_navigation_next(PAGE_SCIENTIFIC) == PAGE_PROGRAMMER);
    CHECK(calculator_navigation_next(PAGE_PROGRAMMER) == PAGE_FORMAT);
    CHECK(calculator_navigation_next(PAGE_FORMAT) == PAGE_TOOLS);
    CHECK(calculator_navigation_next(PAGE_TOOLS) == PAGE_SYMBOLS);
    CHECK(calculator_navigation_next(PAGE_SYMBOLS) == PAGE_LOGIC);
    CHECK(calculator_navigation_next(PAGE_LOGIC) == PAGE_UNITS);
    CHECK(calculator_navigation_next(PAGE_UNITS) == PAGE_COMPLEX);
    CHECK(calculator_navigation_next(PAGE_COMPLEX) == PAGE_BASIC);
    CHECK(calculator_navigation_next(PAGE_GRAPH) == PAGE_BASIC);

    CHECK(calculator_page_accepts_expression(PAGE_BASIC));
    CHECK(calculator_page_accepts_expression(PAGE_SCIENTIFIC));
    CHECK(calculator_page_accepts_expression(PAGE_TOOLS));
    CHECK(!calculator_page_accepts_expression(PAGE_SYMBOLS));
    CHECK(!calculator_page_accepts_expression(PAGE_PROGRAMMER));
    CHECK(!calculator_page_accepts_expression(PAGE_FORMAT));
    CHECK(!calculator_page_accepts_expression(PAGE_GRAPH));
    CHECK(!calculator_page_accepts_expression(PAGE_LOGIC));
    CHECK(!calculator_page_accepts_expression(PAGE_UNITS));
    CHECK(!calculator_page_accepts_expression(PAGE_COMPLEX));

    CHECK(calculator_page_accepts_evaluate(PAGE_PROGRAMMER));
    CHECK(calculator_page_accepts_evaluate(PAGE_BASIC));
    CHECK(!calculator_page_accepts_evaluate(PAGE_GRAPH));
    CHECK(!calculator_page_accepts_evaluate(PAGE_LOGIC));
    CHECK(!calculator_page_accepts_evaluate(PAGE_UNITS));
    CHECK(!calculator_page_accepts_evaluate(PAGE_COMPLEX));
    CHECK(strcmp(calculator_page_message(PAGE_FORMAT), "NUMBER FORMATS") == 0);

    puts("navigation tests passed");
    return 0;
}
