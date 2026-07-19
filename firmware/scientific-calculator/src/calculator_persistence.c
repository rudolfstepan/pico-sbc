#include "calculator_persistence.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define RECORD_HEADER_SIZE 20u
#define LEGACY_PERSISTENCE_VERSION 1u

static const uint8_t record_magic[4] = {'P', 'C', 'A', 'L'};

typedef struct {
    uint8_t *data;
    size_t capacity;
    size_t position;
    bool valid;
} writer_t;

typedef struct {
    const uint8_t *data;
    size_t size;
    size_t position;
    bool valid;
} reader_t;

static void write_bytes(writer_t *writer, const void *value, size_t size) {
    if (!writer->valid || size > writer->capacity - writer->position) {
        writer->valid = false;
        return;
    }
    memcpy(writer->data + writer->position, value, size);
    writer->position += size;
}

static void write_u8(writer_t *writer, uint8_t value) {
    write_bytes(writer, &value, sizeof value);
}

static void write_u16(writer_t *writer, uint16_t value) {
    uint8_t bytes[2] = {(uint8_t)value, (uint8_t)(value >> 8)};
    write_bytes(writer, bytes, sizeof bytes);
}

static void write_u32(writer_t *writer, uint32_t value) {
    uint8_t bytes[4] = {
        (uint8_t)value, (uint8_t)(value >> 8),
        (uint8_t)(value >> 16), (uint8_t)(value >> 24)
    };
    write_bytes(writer, bytes, sizeof bytes);
}

static void write_u64(writer_t *writer, uint64_t value) {
    for (unsigned int shift = 0; shift < 64; shift += 8) {
        write_u8(writer, (uint8_t)(value >> shift));
    }
}

static void write_double(writer_t *writer, double value) {
    uint64_t bits = 0;
    memcpy(&bits, &value, sizeof bits);
    write_u64(writer, bits);
}

static void read_bytes(reader_t *reader, void *value, size_t size) {
    if (!reader->valid || size > reader->size - reader->position) {
        reader->valid = false;
        memset(value, 0, size);
        return;
    }
    memcpy(value, reader->data + reader->position, size);
    reader->position += size;
}

static uint8_t read_u8(reader_t *reader) {
    uint8_t value = 0;
    read_bytes(reader, &value, sizeof value);
    return value;
}

static uint16_t read_u16(reader_t *reader) {
    uint16_t value = read_u8(reader);
    value |= (uint16_t)read_u8(reader) << 8;
    return value;
}

static uint32_t read_u32(reader_t *reader) {
    uint32_t value = read_u8(reader);
    value |= (uint32_t)read_u8(reader) << 8;
    value |= (uint32_t)read_u8(reader) << 16;
    value |= (uint32_t)read_u8(reader) << 24;
    return value;
}

static uint64_t read_u64(reader_t *reader) {
    uint64_t value = 0;
    for (unsigned int shift = 0; shift < 64; shift += 8) {
        value |= (uint64_t)read_u8(reader) << shift;
    }
    return value;
}

static double read_double(reader_t *reader) {
    uint64_t bits = read_u64(reader);
    double value = 0.0;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static bool string_is_terminated(const char *text, size_t capacity) {
    return memchr(text, '\0', capacity) != NULL;
}

static bool format_bits_valid(unsigned int bits) {
    return bits == 8u || bits == 16u || bits == 32u || bits == 64u;
}

static bool base_valid(programmer_base_t base) {
    return base == PROGRAMMER_BIN || base == PROGRAMMER_DEC ||
           base == PROGRAMMER_HEX;
}

static bool graph_is_valid(const graph_model_t *graph,
                           const calculator_symbols_t *symbols) {
    if (graph->selected_function >= GRAPH_FUNCTION_COUNT ||
        graph->view < GRAPH_VIEW_PLOT || graph->view > GRAPH_VIEW_RANGE ||
        !(graph->x_span > 0.0) || !(graph->y_span > 0.0) ||
        !(graph->table_step > 0.0) ||
        !(graph->analysis_tolerance > 0.0) ||
        graph->analysis_max_iterations == 0) {
        return false;
    }
    const double values[] = {
        graph->x_center, graph->y_center, graph->x_span, graph->y_span,
        graph->trace_x, graph->table_x, graph->table_step,
        graph->analysis_left, graph->analysis_right,
        graph->analysis_tolerance
    };
    for (size_t i = 0; i < sizeof values / sizeof values[0]; ++i) {
        if (!isfinite(values[i])) return false;
    }
    for (size_t i = 0; i < GRAPH_FUNCTION_COUNT; ++i) {
        if (!string_is_terminated(graph->functions[i].expression,
                                  sizeof graph->functions[i].expression) ||
            strcmp(graph->functions[i].expression,
                   symbols->functions[i]) != 0) {
            return false;
        }
    }
    return true;
}

static bool statistics_is_valid(const statistics_dataset_t *statistics) {
    if (statistics->count > STATISTICS_CAPACITY) return false;
    for (size_t i = 0; i < STATISTICS_CAPACITY; ++i) {
        if (!isfinite(statistics->x[i]) || !isfinite(statistics->y[i])) {
            return false;
        }
    }
    return true;
}

static bool state_is_valid(const calculator_persisted_state_t *state) {
    if (!state || state->page < PAGE_BASIC || state->page > PAGE_STATISTICS ||
        !format_bits_valid(state->format_bits) ||
        state->fixed_fraction_bits >= state->format_bits ||
        !isfinite(state->ans) || !isfinite(state->memory_value) ||
        !base_valid(state->programmer_base) ||
        state->programmer_selected_bit >= state->format_bits ||
        state->history_count > CALCULATOR_PERSISTENCE_HISTORY_CAPACITY ||
        (state->history_count && state->history_index >= state->history_count) ||
        calculator_symbols_have_cycle(&state->symbols)) {
        return false;
    }
    for (size_t i = 0; i < CALCULATOR_VARIABLE_COUNT; ++i) {
        if (!isfinite(state->symbols.variables[i])) return false;
    }
    for (size_t i = 0; i < CALCULATOR_USER_FUNCTION_COUNT; ++i) {
        if (!string_is_terminated(state->symbols.functions[i],
                                  sizeof state->symbols.functions[i])) {
            return false;
        }
    }
    for (size_t i = 0; i < CALCULATOR_FAVORITE_COUNT; ++i) {
        if (!string_is_terminated(state->symbols.favorites[i],
                                  sizeof state->symbols.favorites[i])) {
            return false;
        }
    }
    for (size_t i = 0; i < state->history_count; ++i) {
        if (!string_is_terminated(state->history[i].formula,
                                  sizeof state->history[i].formula) ||
            !string_is_terminated(state->history[i].result,
                                  sizeof state->history[i].result) ||
            !isfinite(state->history[i].value)) {
            return false;
        }
    }
    return graph_is_valid(&state->graph, &state->symbols) &&
           statistics_is_valid(&state->statistics);
}

void calculator_persistence_defaults(calculator_persisted_state_t *state) {
    memset(state, 0, sizeof *state);
    state->degrees = true;
    state->page = PAGE_BASIC;
    state->format_bits = 32;
    state->fixed_fraction_bits = 16;
    state->programmer_base = PROGRAMMER_DEC;
    calculator_symbols_init(&state->symbols);
    graph_model_init(&state->graph);
    graph_model_reset_range(&state->graph);
}

static void write_payload(writer_t *writer,
                          const calculator_persisted_state_t *state) {
    write_u8(writer, state->degrees ? 1u : 0u);
    write_u8(writer, (uint8_t)state->page);
    write_u8(writer, (uint8_t)state->format_bits);
    write_u8(writer, (uint8_t)state->fixed_fraction_bits);
    write_u8(writer, (uint8_t)state->programmer_base);
    write_u8(writer, state->programmer_signed ? 1u : 0u);
    write_u8(writer, (uint8_t)state->programmer_selected_bit);
    write_u8(writer, 0);
    write_double(writer, state->ans);
    write_double(writer, state->memory_value);
    write_u64(writer, state->programmer_value);

    for (size_t i = 0; i < CALCULATOR_VARIABLE_COUNT; ++i) {
        write_double(writer, state->symbols.variables[i]);
    }
    write_bytes(writer, state->symbols.functions,
                sizeof state->symbols.functions);
    write_bytes(writer, state->symbols.favorites,
                sizeof state->symbols.favorites);

    write_u8(writer, (uint8_t)state->history_count);
    write_u8(writer, (uint8_t)state->history_index);
    for (unsigned int i = 0; i < 6; ++i) write_u8(writer, 0);
    for (size_t i = 0; i < CALCULATOR_PERSISTENCE_HISTORY_CAPACITY; ++i) {
        write_bytes(writer, state->history[i].formula,
                    sizeof state->history[i].formula);
        write_bytes(writer, state->history[i].result,
                    sizeof state->history[i].result);
        write_double(writer, state->history[i].value);
    }

    unsigned int active_mask = 0;
    for (size_t i = 0; i < GRAPH_FUNCTION_COUNT; ++i) {
        if (state->graph.functions[i].active &&
            state->graph.functions[i].expression[0]) {
            active_mask |= 1u << i;
        }
    }
    write_u8(writer, (uint8_t)active_mask);
    write_u8(writer, (uint8_t)state->graph.selected_function);
    write_u8(writer, (uint8_t)state->graph.view);
    write_u8(writer, state->graph.trace_enabled ? 1u : 0u);
    write_u8(writer, state->graph.custom_analysis_interval ? 1u : 0u);
    write_u8(writer, 0);
    write_u8(writer, 0);
    write_u8(writer, 0);
    write_double(writer, state->graph.x_center);
    write_double(writer, state->graph.y_center);
    write_double(writer, state->graph.x_span);
    write_double(writer, state->graph.y_span);
    write_double(writer, state->graph.trace_x);
    write_double(writer, state->graph.table_x);
    write_double(writer, state->graph.table_step);
    write_double(writer, state->graph.analysis_left);
    write_double(writer, state->graph.analysis_right);
    write_double(writer, state->graph.analysis_tolerance);
    write_u32(writer, state->graph.analysis_max_iterations);

    write_u8(writer, (uint8_t)state->statistics.count);
    write_u8(writer, state->statistics.two_variable ? 1u : 0u);
    for (unsigned int i = 0; i < 6; ++i) write_u8(writer, 0);
    for (size_t i = 0; i < STATISTICS_CAPACITY; ++i) {
        write_double(writer, state->statistics.x[i]);
        write_double(writer, state->statistics.y[i]);
    }
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned int bit = 0; bit < 8; ++bit) {
            uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & mask);
        }
    }
    return crc;
}

static uint32_t record_crc(const uint8_t *record, size_t payload_size) {
    uint32_t crc = UINT32_MAX;
    crc = crc32_update(crc, record, 16);
    crc = crc32_update(crc, record + RECORD_HEADER_SIZE, payload_size);
    return ~crc;
}

bool calculator_persistence_encode(
    const calculator_persisted_state_t *state, uint32_t sequence,
    uint8_t *record, size_t capacity, size_t *record_size) {
    if (record_size) *record_size = 0;
    if (!record || capacity < RECORD_HEADER_SIZE || !state_is_valid(state)) {
        return false;
    }

    writer_t writer = {
        .data = record,
        .capacity = capacity,
        .valid = true,
    };
    write_bytes(&writer, record_magic, sizeof record_magic);
    write_u16(&writer, CALCULATOR_PERSISTENCE_VERSION);
    write_u16(&writer, RECORD_HEADER_SIZE);
    size_t payload_size_position = writer.position;
    write_u32(&writer, 0);
    write_u32(&writer, sequence);
    size_t crc_position = writer.position;
    write_u32(&writer, 0);
    size_t payload_start = writer.position;
    write_payload(&writer, state);
    if (!writer.valid) return false;

    size_t payload_size = writer.position - payload_start;
    writer_t patch = {
        .data = record + payload_size_position,
        .capacity = 4,
        .valid = true,
    };
    write_u32(&patch, (uint32_t)payload_size);
    uint32_t crc = record_crc(record, payload_size);
    patch.data = record + crc_position;
    patch.position = 0;
    write_u32(&patch, crc);
    if (record_size) *record_size = writer.position;
    return true;
}

static void read_payload(reader_t *reader, uint16_t version,
                         calculator_persisted_state_t *state) {
    calculator_persistence_defaults(state);
    state->degrees = read_u8(reader) != 0;
    state->page = (calc_page_t)read_u8(reader);
    state->format_bits = read_u8(reader);
    state->fixed_fraction_bits = read_u8(reader);
    state->programmer_base = (programmer_base_t)read_u8(reader);
    state->programmer_signed = read_u8(reader) != 0;
    state->programmer_selected_bit = read_u8(reader);
    (void)read_u8(reader);
    state->ans = read_double(reader);
    state->memory_value = read_double(reader);
    state->programmer_value = read_u64(reader);

    for (size_t i = 0; i < CALCULATOR_VARIABLE_COUNT; ++i) {
        state->symbols.variables[i] = read_double(reader);
    }
    read_bytes(reader, state->symbols.functions,
               sizeof state->symbols.functions);
    read_bytes(reader, state->symbols.favorites,
               sizeof state->symbols.favorites);

    state->history_count = read_u8(reader);
    state->history_index = read_u8(reader);
    for (unsigned int i = 0; i < 6; ++i) (void)read_u8(reader);
    for (size_t i = 0; i < CALCULATOR_PERSISTENCE_HISTORY_CAPACITY; ++i) {
        read_bytes(reader, state->history[i].formula,
                   sizeof state->history[i].formula);
        read_bytes(reader, state->history[i].result,
                   sizeof state->history[i].result);
        state->history[i].value = read_double(reader);
    }

    unsigned int active_mask = read_u8(reader);
    state->graph.selected_function = read_u8(reader);
    state->graph.view = (graph_view_t)read_u8(reader);
    state->graph.trace_enabled = read_u8(reader) != 0;
    state->graph.custom_analysis_interval = read_u8(reader) != 0;
    for (unsigned int i = 0; i < 3; ++i) (void)read_u8(reader);
    state->graph.x_center = read_double(reader);
    state->graph.y_center = read_double(reader);
    state->graph.x_span = read_double(reader);
    state->graph.y_span = read_double(reader);
    state->graph.trace_x = read_double(reader);
    state->graph.table_x = read_double(reader);
    state->graph.table_step = read_double(reader);
    state->graph.analysis_left = read_double(reader);
    state->graph.analysis_right = read_double(reader);
    state->graph.analysis_tolerance = read_double(reader);
    state->graph.analysis_max_iterations = read_u32(reader);
    for (size_t i = 0; i < GRAPH_FUNCTION_COUNT; ++i) {
        snprintf(state->graph.functions[i].expression,
                 sizeof state->graph.functions[i].expression, "%s",
                 state->symbols.functions[i]);
        state->graph.functions[i].active =
            state->symbols.functions[i][0] && (active_mask & (1u << i));
    }
    state->graph.analysis_text[0] = '\0';

    if (version >= 2u) {
        state->statistics.count = read_u8(reader);
        state->statistics.two_variable = read_u8(reader) != 0;
        for (unsigned int i = 0; i < 6; ++i) (void)read_u8(reader);
        for (size_t i = 0; i < STATISTICS_CAPACITY; ++i) {
            state->statistics.x[i] = read_double(reader);
            state->statistics.y[i] = read_double(reader);
        }
    }
}

static bool record_is_erased(const uint8_t *record, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (record[i] != 0xffu) return false;
    }
    return true;
}

calculator_persistence_status_t calculator_persistence_decode(
    const uint8_t *record, size_t record_size,
    calculator_persisted_state_t *state, uint32_t *sequence) {
    if (sequence) *sequence = 0;
    if (!record || record_size < RECORD_HEADER_SIZE) {
        return CALCULATOR_PERSISTENCE_CORRUPT;
    }
    if (record_is_erased(record, record_size)) {
        return CALCULATOR_PERSISTENCE_EMPTY;
    }

    reader_t header = {
        .data = record,
        .size = record_size,
        .valid = true,
    };
    uint8_t magic[4];
    read_bytes(&header, magic, sizeof magic);
    uint16_t version = read_u16(&header);
    uint16_t header_size = read_u16(&header);
    uint32_t payload_size = read_u32(&header);
    uint32_t decoded_sequence = read_u32(&header);
    uint32_t expected_crc = read_u32(&header);
    if (!header.valid || memcmp(magic, record_magic, sizeof magic) != 0 ||
        header_size != RECORD_HEADER_SIZE ||
        payload_size > record_size - RECORD_HEADER_SIZE) {
        return CALCULATOR_PERSISTENCE_CORRUPT;
    }
    if (version != LEGACY_PERSISTENCE_VERSION &&
        version != CALCULATOR_PERSISTENCE_VERSION) {
        return CALCULATOR_PERSISTENCE_UNSUPPORTED;
    }
    if (record_crc(record, payload_size) != expected_crc) {
        return CALCULATOR_PERSISTENCE_CORRUPT;
    }

    calculator_persisted_state_t decoded;
    reader_t payload = {
        .data = record + RECORD_HEADER_SIZE,
        .size = payload_size,
        .valid = true,
    };
    read_payload(&payload, version, &decoded);
    if (!payload.valid || payload.position != payload.size ||
        !state_is_valid(&decoded)) {
        return CALCULATOR_PERSISTENCE_CORRUPT;
    }
    if (state) *state = decoded;
    if (sequence) *sequence = decoded_sequence;
    return CALCULATOR_PERSISTENCE_VALID;
}

static bool sequence_is_newer(uint32_t first, uint32_t second) {
    return (int32_t)(first - second) > 0;
}

calculator_persistence_status_t calculator_persistence_select(
    const uint8_t *first, size_t first_size,
    const uint8_t *second, size_t second_size,
    calculator_persisted_state_t *state, uint32_t *sequence,
    size_t *selected_slot) {
    calculator_persisted_state_t states[2];
    uint32_t sequences[2] = {0, 0};
    calculator_persistence_status_t statuses[2] = {
        calculator_persistence_decode(first, first_size, &states[0],
                                      &sequences[0]),
        calculator_persistence_decode(second, second_size, &states[1],
                                      &sequences[1]),
    };
    bool valid_first = statuses[0] == CALCULATOR_PERSISTENCE_VALID;
    bool valid_second = statuses[1] == CALCULATOR_PERSISTENCE_VALID;
    if (valid_first || valid_second) {
        size_t selected = !valid_first ? 1u :
            (!valid_second ? 0u :
             (sequence_is_newer(sequences[1], sequences[0]) ? 1u : 0u));
        if (state) *state = states[selected];
        if (sequence) *sequence = sequences[selected];
        if (selected_slot) *selected_slot = selected;
        return CALCULATOR_PERSISTENCE_VALID;
    }
    if (statuses[0] == CALCULATOR_PERSISTENCE_EMPTY &&
        statuses[1] == CALCULATOR_PERSISTENCE_EMPTY) {
        return CALCULATOR_PERSISTENCE_EMPTY;
    }
    if (statuses[0] == CALCULATOR_PERSISTENCE_UNSUPPORTED ||
        statuses[1] == CALCULATOR_PERSISTENCE_UNSUPPORTED) {
        return CALCULATOR_PERSISTENCE_UNSUPPORTED;
    }
    return CALCULATOR_PERSISTENCE_CORRUPT;
}
