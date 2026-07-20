#include "calculator_persistence.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static void fill_state(calculator_persisted_state_t *state) {
    calculator_persistence_defaults(state);
    state->degrees = false;
    state->precision = CALCULATOR_PRECISION_ULTRA;
    state->page = PAGE_STATISTICS;
    state->format_bits = 64;
    state->fixed_fraction_bits = 24;
    state->ans = 12.5;
    snprintf(state->ans_text, sizeof state->ans_text,
             "12.5000000000000000000000000000000000000000000001");
    state->memory_value = -7.25;
    snprintf(state->memory_text, sizeof state->memory_text, "-7.25");
    state->programmer_base = PROGRAMMER_HEX;
    state->programmer_value = UINT64_C(0x123456789abcdef0);
    state->programmer_signed = true;
    state->programmer_selected_bit = 17;
    calculator_symbols_set_variable_precise(
        &state->symbols, 0, 42.0,
        "42.0000000000000000000000000000000000000000000001");
    calculator_symbols_set_function(&state->symbols, 0, "sin(x)+A");
    calculator_symbols_set_favorite(&state->symbols, 0, "f1(");
    graph_model_set_function(&state->graph, 0, "sin(x)+A");
    state->graph.x_center = 3.0;
    state->graph.y_center = -2.0;
    state->graph.x_span = 8.0;
    state->graph.y_span = 4.0;
    state->graph.trace_enabled = true;
    state->graph.trace_x = 1.5;
    state->graph.table_x = -1.0;
    state->graph.table_step = 0.25;
    state->history_count = 2;
    state->history_index = 1;
    snprintf(state->history[0].formula, sizeof state->history[0].formula,
             "6*7");
    snprintf(state->history[0].result, sizeof state->history[0].result,
             "42");
    state->history[0].value = 42.0;
    snprintf(state->history[1].formula, sizeof state->history[1].formula,
             "f1(30)");
    snprintf(state->history[1].result, sizeof state->history[1].result,
             "42.5");
    state->history[1].value = 42.5;
    state->statistics.two_variable = true;
    state->statistics.count = 3;
    state->statistics.x[0] = 1.0;
    state->statistics.y[0] = 3.0;
    state->statistics.x[1] = 2.0;
    state->statistics.y[1] = 5.0;
    state->statistics.x[2] = 3.0;
    state->statistics.y[2] = 7.0;
    state->basic_program.count = 3;
    state->basic_program.lines[0].number = 10;
    snprintf(state->basic_program.lines[0].text,
             sizeof state->basic_program.lines[0].text, "FOR I=1 TO 3");
    state->basic_program.lines[1].number = 20;
    snprintf(state->basic_program.lines[1].text,
             sizeof state->basic_program.lines[1].text, "PRINT I");
    state->basic_program.lines[2].number = 30;
    snprintf(state->basic_program.lines[2].text,
             sizeof state->basic_program.lines[2].text, "NEXT I");
    (void)circuit_model_toggle_input(&state->circuit, 0u);
    (void)circuit_model_toggle_input(&state->circuit, 1u);
    state->circuit_viewport_x = 128u;
    state->circuit_viewport_y = 64u;
    state->circuit_zoom_index = 2u;
}

int main(void) {
    calculator_persisted_state_t original;
    fill_state(&original);

    uint8_t first[CALCULATOR_PERSISTENCE_RECORD_CAPACITY];
    uint8_t second[CALCULATOR_PERSISTENCE_RECORD_CAPACITY];
    memset(first, 0xff, sizeof first);
    memset(second, 0xff, sizeof second);
    size_t first_size = 0;
    CHECK(calculator_persistence_encode(&original, 41, first, sizeof first,
                                        &first_size));
    CHECK(first_size > 1500 && first_size < sizeof first);

    calculator_persisted_state_t decoded;
    uint32_t sequence = 0;
    CHECK(calculator_persistence_decode(first, first_size, &decoded,
                                        &sequence) ==
          CALCULATOR_PERSISTENCE_VALID);
    CHECK(sequence == 41);
    CHECK(!decoded.degrees && decoded.page == PAGE_STATISTICS);
    CHECK(decoded.precision == CALCULATOR_PRECISION_ULTRA);
    CHECK(decoded.format_bits == 64 && decoded.fixed_fraction_bits == 24);
    CHECK(strcmp(decoded.ans_text,
                 "12.5000000000000000000000000000000000000000000001") == 0);
    CHECK(fabs(decoded.memory_value + 7.25) < 1e-12);
    CHECK(strcmp(decoded.memory_text, "-7.25") == 0);
    CHECK(decoded.programmer_value == UINT64_C(0x123456789abcdef0));
    CHECK(decoded.programmer_signed && decoded.programmer_selected_bit == 17);
    CHECK(strcmp(decoded.symbols.functions[0], "sin(x)+A") == 0);
    CHECK(strcmp(decoded.symbols.variable_text[0],
                 "42.0000000000000000000000000000000000000000000001") == 0);
    CHECK(decoded.history_count == 2 && decoded.history_index == 1);
    CHECK(strcmp(decoded.history[1].formula, "f1(30)") == 0);
    CHECK(fabs(decoded.graph.x_center - 3.0) < 1e-12);
    CHECK(decoded.graph.trace_enabled);
    CHECK(decoded.statistics.two_variable && decoded.statistics.count == 3);
    CHECK(decoded.statistics.x[1] == 2.0 &&
          decoded.statistics.y[2] == 7.0);
    CHECK(decoded.basic_program.count == 3);
    CHECK(decoded.basic_program.lines[1].number == 20);
    CHECK(strcmp(decoded.basic_program.lines[2].text, "NEXT I") == 0);
    CHECK(decoded.circuit_viewport_x == 128u &&
          decoded.circuit_viewport_y == 64u);
    CHECK(decoded.circuit_zoom_index == 2u);
    CHECK(decoded.circuit.nodes[0].input_value);
    CHECK(decoded.circuit.nodes[3].output_value);

    uint8_t canonical[CALCULATOR_PERSISTENCE_RECORD_CAPACITY];
    size_t canonical_size = 0;
    CHECK(calculator_persistence_encode(&decoded, sequence, canonical,
                                        sizeof canonical, &canonical_size));
    CHECK(canonical_size == first_size &&
          memcmp(canonical, first, first_size) == 0);

    size_t second_size = 0;
    original.memory_value = 99.0;
    snprintf(original.memory_text, sizeof original.memory_text, "99");
    CHECK(calculator_persistence_encode(&original, 42, second, sizeof second,
                                        &second_size));
    size_t selected = 99;
    CHECK(calculator_persistence_select(
              first, first_size, second, second_size, &decoded, &sequence,
              &selected) == CALCULATOR_PERSISTENCE_VALID);
    CHECK(selected == 1 && sequence == 42 && decoded.memory_value == 99.0);

    second[second_size - 1] ^= 0x01u;
    CHECK(calculator_persistence_select(
              first, first_size, second, second_size, &decoded, &sequence,
              &selected) == CALCULATOR_PERSISTENCE_VALID);
    CHECK(selected == 0 && sequence == 41 && decoded.memory_value == -7.25);

    first[30] ^= 0x80u;
    CHECK(calculator_persistence_select(
              first, first_size, second, second_size, &decoded, &sequence,
              &selected) == CALCULATOR_PERSISTENCE_CORRUPT);

    memset(first, 0xff, sizeof first);
    memset(second, 0xff, sizeof second);
    CHECK(calculator_persistence_select(
              first, sizeof first, second, sizeof second, &decoded, &sequence,
              &selected) == CALCULATOR_PERSISTENCE_EMPTY);

    fill_state(&original);
    CHECK(calculator_persistence_encode(&original, UINT32_MAX, first,
                                        sizeof first, &first_size));
    original.memory_value = 77.0;
    snprintf(original.memory_text, sizeof original.memory_text, "77");
    CHECK(calculator_persistence_encode(&original, 0, second,
                                        sizeof second, &second_size));
    CHECK(calculator_persistence_select(
              first, first_size, second, second_size, &decoded, &sequence,
              &selected) == CALCULATOR_PERSISTENCE_VALID);
    CHECK(selected == 1 && sequence == 0 && decoded.memory_value == 77.0);

    first[4] = 5u;
    first[5] = 0;
    CHECK(calculator_persistence_decode(first, first_size, &decoded,
                                        &sequence) ==
          CALCULATOR_PERSISTENCE_UNSUPPORTED);

    fill_state(&original);
    original.graph.x_span = 0.0;
    CHECK(!calculator_persistence_encode(&original, 1, first, sizeof first,
                                         &first_size));

    fill_state(&original);
    original.circuit.nodes[3].type = CIRCUIT_GATE_NOT;
    original.circuit.wires[0].source = 3u;
    original.circuit.wires[0].destination = 2u;
    original.circuit.wires[0].destination_input = 0u;
    CHECK(!calculator_persistence_encode(&original, 1, first, sizeof first,
                                         &first_size));

    puts("calculator persistence tests passed");
    return 0;
}
