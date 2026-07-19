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

#define STATISTICS_PAYLOAD_SIZE \
    (8u + STATISTICS_CAPACITY * 2u * sizeof(double))

static void put_u32(uint8_t *destination, uint32_t value) {
    destination[0] = (uint8_t)value;
    destination[1] = (uint8_t)(value >> 8);
    destination[2] = (uint8_t)(value >> 16);
    destination[3] = (uint8_t)(value >> 24);
}

static uint32_t test_crc32_update(uint32_t crc, const uint8_t *data,
                                  size_t size) {
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned int bit = 0; bit < 8; ++bit) {
            uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & mask);
        }
    }
    return crc;
}

static uint32_t test_record_crc(const uint8_t *record, size_t payload_size) {
    uint32_t crc = test_crc32_update(UINT32_MAX, record, 16);
    crc = test_crc32_update(crc, record + 20, payload_size);
    return ~crc;
}

static void fill_state(calculator_persisted_state_t *state) {
    calculator_persistence_defaults(state);
    state->degrees = false;
    state->page = PAGE_STATISTICS;
    state->format_bits = 64;
    state->fixed_fraction_bits = 24;
    state->ans = 12.5;
    state->memory_value = -7.25;
    state->programmer_base = PROGRAMMER_HEX;
    state->programmer_value = UINT64_C(0x123456789abcdef0);
    state->programmer_signed = true;
    state->programmer_selected_bit = 17;
    calculator_symbols_set_variable(&state->symbols, 0, 42.0);
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
    CHECK(decoded.format_bits == 64 && decoded.fixed_fraction_bits == 24);
    CHECK(fabs(decoded.memory_value + 7.25) < 1e-12);
    CHECK(decoded.programmer_value == UINT64_C(0x123456789abcdef0));
    CHECK(decoded.programmer_signed && decoded.programmer_selected_bit == 17);
    CHECK(strcmp(decoded.symbols.functions[0], "sin(x)+A") == 0);
    CHECK(decoded.history_count == 2 && decoded.history_index == 1);
    CHECK(strcmp(decoded.history[1].formula, "f1(30)") == 0);
    CHECK(fabs(decoded.graph.x_center - 3.0) < 1e-12);
    CHECK(decoded.graph.trace_enabled);
    CHECK(decoded.statistics.two_variable && decoded.statistics.count == 3);
    CHECK(decoded.statistics.x[1] == 2.0 &&
          decoded.statistics.y[2] == 7.0);

    uint8_t canonical[CALCULATOR_PERSISTENCE_RECORD_CAPACITY];
    size_t canonical_size = 0;
    CHECK(calculator_persistence_encode(&decoded, sequence, canonical,
                                        sizeof canonical, &canonical_size));
    CHECK(canonical_size == first_size &&
          memcmp(canonical, first, first_size) == 0);

    uint8_t legacy[CALCULATOR_PERSISTENCE_RECORD_CAPACITY];
    memcpy(legacy, first, first_size);
    size_t legacy_payload_size = first_size - 20u - STATISTICS_PAYLOAD_SIZE;
    legacy[4] = 1u;
    legacy[5] = 0u;
    legacy[21] = (uint8_t)PAGE_COMPLEX;
    put_u32(legacy + 8, (uint32_t)legacy_payload_size);
    put_u32(legacy + 16, test_record_crc(legacy, legacy_payload_size));
    CHECK(calculator_persistence_decode(
              legacy, 20u + legacy_payload_size, &decoded, &sequence) ==
          CALCULATOR_PERSISTENCE_VALID);
    CHECK(decoded.page == PAGE_COMPLEX && decoded.statistics.count == 0 &&
          !decoded.statistics.two_variable);

    size_t second_size = 0;
    original.memory_value = 99.0;
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
    CHECK(calculator_persistence_encode(&original, 0, second,
                                        sizeof second, &second_size));
    CHECK(calculator_persistence_select(
              first, first_size, second, second_size, &decoded, &sequence,
              &selected) == CALCULATOR_PERSISTENCE_VALID);
    CHECK(selected == 1 && sequence == 0 && decoded.memory_value == 77.0);

    first[4] = (uint8_t)(CALCULATOR_PERSISTENCE_VERSION + 1u);
    first[5] = 0;
    CHECK(calculator_persistence_decode(first, first_size, &decoded,
                                        &sequence) ==
          CALCULATOR_PERSISTENCE_UNSUPPORTED);

    fill_state(&original);
    original.graph.x_span = 0.0;
    CHECK(!calculator_persistence_encode(&original, 1, first, sizeof first,
                                         &first_size));

    puts("calculator persistence tests passed");
    return 0;
}
