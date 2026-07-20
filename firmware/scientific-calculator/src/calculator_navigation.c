#include "calculator_navigation.h"

#include <stddef.h>
#include <string.h>

calc_page_t calculator_navigation_next(calc_page_t page) {
    (void)page;
    return PAGE_LAUNCHER;
}

calc_page_t calculator_navigation_target(const char *token) {
    static const struct {
        const char *token;
        calc_page_t page;
    } routes[] = {
        {"CALC", PAGE_BASIC}, {"PROG", PAGE_PROGRAMMER},
        {"FORMAT", PAGE_FORMAT}, {"TOOLS", PAGE_TOOLS},
        {"SYMBOLS", PAGE_SYMBOLS}, {"GRAPH", PAGE_GRAPH},
        {"LOGIC", PAGE_LOGIC}, {"UNITS", PAGE_UNITS},
        {"COMPLEX", PAGE_COMPLEX}, {"STATS", PAGE_STATISTICS},
        {"BASIC", PAGE_BASIC_PROGRAM}, {"SETTINGS", PAGE_SETTINGS},
        {"NUMBER", PAGE_NUMBER_THEORY}, {"CIRCUIT", PAGE_CIRCUIT},
    };
    for (size_t i = 0; i < sizeof routes / sizeof routes[0]; ++i) {
        if (strcmp(token, routes[i].token) == 0) return routes[i].page;
    }
    return PAGE_LAUNCHER;
}

bool calculator_page_accepts_expression(calc_page_t page) {
    return page == PAGE_BASIC || page == PAGE_SCIENTIFIC || page == PAGE_TOOLS;
}

bool calculator_page_accepts_evaluate(calc_page_t page) {
    return page == PAGE_PROGRAMMER || calculator_page_accepts_expression(page);
}

const char *calculator_page_message(calc_page_t page) {
    switch (page) {
        case PAGE_BASIC: return "BASIC KEYS";
        case PAGE_SCIENTIFIC: return "SCI FUNCTIONS";
        case PAGE_PROGRAMMER: return "64-BIT MODE";
        case PAGE_FORMAT: return "NUMBER FORMATS";
        case PAGE_TOOLS: return "TOOLS";
        case PAGE_SYMBOLS: return "VARIABLES AND FUNCTIONS";
        case PAGE_GRAPH: return "GRAPH";
        case PAGE_LOGIC: return "DIGITAL LOGIC";
        case PAGE_UNITS: return "UNITS AND CONSTANTS";
        case PAGE_COMPLEX: return "COMPLEX NUMBERS";
        case PAGE_STATISTICS: return "STATISTICS";
        case PAGE_BASIC_PROGRAM: return "BASIC PROGRAMMING";
        case PAGE_LAUNCHER: return "APPS";
        case PAGE_SETTINGS: return "SETTINGS";
        case PAGE_NUMBER_THEORY: return "NUMBER THEORY";
        case PAGE_CIRCUIT: return "CIRCUIT EDITOR";
        default: return "READY";
    }
}
