#ifndef CALCULATOR_PERSISTENCE_H
#define CALCULATOR_PERSISTENCE_H

#include "calculator_symbols.h"
#include "calculator_ui_types.h"
#include "expression_editor.h"
#include "graph_model.h"
#include "programmer_engine.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CALCULATOR_PERSISTENCE_VERSION 1u
#define CALCULATOR_PERSISTENCE_RECORD_CAPACITY 2048u
#define CALCULATOR_PERSISTENCE_HISTORY_CAPACITY 8u
#define CALCULATOR_PERSISTENCE_RESULT_CAPACITY 32u

typedef struct {
    char formula[EXPRESSION_EDITOR_CAPACITY];
    char result[CALCULATOR_PERSISTENCE_RESULT_CAPACITY];
    double value;
} calculator_persisted_history_entry_t;

typedef struct {
    bool degrees;
    calc_page_t page;
    unsigned int format_bits;
    unsigned int fixed_fraction_bits;
    double ans;
    double memory_value;
    programmer_base_t programmer_base;
    uint64_t programmer_value;
    calculator_symbols_t symbols;
    calculator_persisted_history_entry_t
        history[CALCULATOR_PERSISTENCE_HISTORY_CAPACITY];
    size_t history_count;
    size_t history_index;
    graph_model_t graph;
} calculator_persisted_state_t;

typedef enum {
    CALCULATOR_PERSISTENCE_VALID,
    CALCULATOR_PERSISTENCE_EMPTY,
    CALCULATOR_PERSISTENCE_CORRUPT,
    CALCULATOR_PERSISTENCE_UNSUPPORTED
} calculator_persistence_status_t;

void calculator_persistence_defaults(calculator_persisted_state_t *state);
bool calculator_persistence_encode(
    const calculator_persisted_state_t *state, uint32_t sequence,
    uint8_t *record, size_t capacity, size_t *record_size);
calculator_persistence_status_t calculator_persistence_decode(
    const uint8_t *record, size_t record_size,
    calculator_persisted_state_t *state, uint32_t *sequence);
calculator_persistence_status_t calculator_persistence_select(
    const uint8_t *first, size_t first_size,
    const uint8_t *second, size_t second_size,
    calculator_persisted_state_t *state, uint32_t *sequence,
    size_t *selected_slot);

#endif
