#include "calculator_navigation.h"

calc_page_t calculator_navigation_next(calc_page_t page) {
    switch (page) {
        case PAGE_BASIC: return PAGE_SCIENTIFIC;
        case PAGE_SCIENTIFIC: return PAGE_PROGRAMMER;
        case PAGE_PROGRAMMER: return PAGE_FORMAT;
        case PAGE_FORMAT: return PAGE_TOOLS;
        case PAGE_TOOLS: return PAGE_SYMBOLS;
        case PAGE_SYMBOLS: return PAGE_LOGIC;
        case PAGE_LOGIC: return PAGE_UNITS;
        case PAGE_UNITS: return PAGE_COMPLEX;
        case PAGE_COMPLEX:
        case PAGE_GRAPH:
        default:
            return PAGE_BASIC;
    }
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
        default: return "READY";
    }
}
