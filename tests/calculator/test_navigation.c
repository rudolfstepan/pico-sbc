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
    for (calc_page_t page = PAGE_BASIC; page <= PAGE_CIRCUIT;
         page = (calc_page_t)(page + 1)) {
        CHECK(calculator_navigation_next(page) == PAGE_LAUNCHER);
    }
    CHECK(calculator_navigation_target("CALC") == PAGE_BASIC);
    CHECK(calculator_navigation_target("GRAPH") == PAGE_GRAPH);
    CHECK(calculator_navigation_target("BASIC") == PAGE_BASIC_PROGRAM);
    CHECK(calculator_navigation_target("NUMBER") == PAGE_NUMBER_THEORY);
    CHECK(calculator_navigation_target("CIRCUIT") == PAGE_CIRCUIT);
    CHECK(calculator_navigation_target("SETTINGS") == PAGE_SETTINGS);
    CHECK(calculator_navigation_target("UNKNOWN") == PAGE_LAUNCHER);

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
    CHECK(!calculator_page_accepts_expression(PAGE_STATISTICS));
    CHECK(!calculator_page_accepts_expression(PAGE_BASIC_PROGRAM));
    CHECK(!calculator_page_accepts_expression(PAGE_LAUNCHER));
    CHECK(!calculator_page_accepts_expression(PAGE_NUMBER_THEORY));
    CHECK(!calculator_page_accepts_expression(PAGE_CIRCUIT));

    CHECK(calculator_page_accepts_evaluate(PAGE_PROGRAMMER));
    CHECK(calculator_page_accepts_evaluate(PAGE_BASIC));
    CHECK(!calculator_page_accepts_evaluate(PAGE_GRAPH));
    CHECK(!calculator_page_accepts_evaluate(PAGE_LOGIC));
    CHECK(!calculator_page_accepts_evaluate(PAGE_UNITS));
    CHECK(!calculator_page_accepts_evaluate(PAGE_COMPLEX));
    CHECK(!calculator_page_accepts_evaluate(PAGE_STATISTICS));
    CHECK(!calculator_page_accepts_evaluate(PAGE_BASIC_PROGRAM));
    CHECK(strcmp(calculator_page_message(PAGE_FORMAT), "NUMBER FORMATS") == 0);
    CHECK(strcmp(calculator_page_message(PAGE_NUMBER_THEORY),
                 "NUMBER THEORY") == 0);
    CHECK(strcmp(calculator_page_message(PAGE_CIRCUIT),
                 "CIRCUIT EDITOR") == 0);

    puts("navigation tests passed");
    return 0;
}
