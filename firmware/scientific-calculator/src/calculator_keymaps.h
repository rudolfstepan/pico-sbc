#ifndef CALCULATOR_KEYMAPS_H
#define CALCULATOR_KEYMAPS_H

#include "calculator_ui_types.h"
#include "graph_model.h"

#include <stddef.h>

const calc_key_t *calculator_keymap(calc_page_t page, size_t *count);
const calc_key_t *calculator_graph_keymap(graph_view_t view, size_t *count);

#endif
