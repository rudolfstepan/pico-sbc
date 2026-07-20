#ifndef CALCULATOR_KEYMAPS_H
#define CALCULATOR_KEYMAPS_H

#include "calculator_ui_types.h"
#include "calculator_units.h"
#include "graph_model.h"

#include <stdbool.h>
#include <stddef.h>

const calc_key_t *calculator_keymap(calc_page_t page, size_t *count);
const calc_key_t *calculator_format_keymap(calculator_format_view_t view,
                                           size_t *count);
const calc_key_t *calculator_graph_keymap(graph_view_t view, size_t *count);
const calc_key_t *calculator_complex_keymap(bool history, size_t *count);
const calc_key_t *calculator_units_keymap(
    calculator_units_view_t view, calculator_units_selector_t selector,
    size_t *count);

#endif
