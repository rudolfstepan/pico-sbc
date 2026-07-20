#ifndef CALCULATOR_NAVIGATION_H
#define CALCULATOR_NAVIGATION_H

#include "calculator_ui_types.h"

#include <stdbool.h>

calc_page_t calculator_navigation_next(calc_page_t page);
calc_page_t calculator_navigation_target(const char *token);
bool calculator_page_accepts_expression(calc_page_t page);
bool calculator_page_accepts_evaluate(calc_page_t page);
const char *calculator_page_message(calc_page_t page);

#endif
