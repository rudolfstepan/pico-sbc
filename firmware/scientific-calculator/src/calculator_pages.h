#ifndef CALCULATOR_PAGES_H
#define CALCULATOR_PAGES_H

#include "calculator_ui_types.h"
#include "calculator_logic.h"
#include "calculator_number_theory.h"
#include "calculator_complex.h"
#include "calculator_precision.h"
#include "calculator_statistics.h"
#include "calculator_units.h"
#include "calculator_symbols.h"
#include "expression_editor.h"
#include "programmer_engine.h"

#include <stdbool.h>
#include <stddef.h>

void calculator_page_render_expression(calc_page_t page, bool degrees,
                                       calculator_precision_t precision,
                                       const char *message,
                                       const expression_editor_t *editor,
                                       const char *result_text);
void calculator_page_render_programmer(const programmer_engine_t *programmer,
                                       const char *message);
void calculator_page_render_format(const programmer_engine_t *programmer,
                                   unsigned int fixed_fraction_bits,
                                   calculator_format_view_t view,
                                   const char *message);
void calculator_page_render_tools(const char *memory_text,
                                  calculator_precision_t precision,
                                  const char *message,
                                  const expression_editor_t *editor,
                                  size_t history_count,
                                  size_t history_index,
                                  const char *history_formula,
                                  const char *history_result,
                                  const char *result_text);
void calculator_page_render_symbols(const calculator_symbols_t *symbols,
                                    size_t selected_function,
                                    const char *message);
void calculator_page_render_logic(const calculator_logic_t *logic,
                                  const char *message);
void calculator_page_render_units(const calculator_units_t *units,
                                  const char *message);
void calculator_page_render_complex(const calculator_complex_t *complex,
                                    bool degrees, const char *message);
void calculator_page_render_statistics(const calculator_statistics_t *stats,
                                       const char *message);
void calculator_page_render_launcher(const char *message);
void calculator_page_render_settings(uint8_t brightness_percent,
                                     bool beep_enabled, bool portrait,
                                     calculator_layout_t default_layout,
                                     calculator_precision_t precision,
                                     const char *message);
void calculator_page_render_number_theory(
    const calculator_number_theory_t *tool, const char *message);

#endif
