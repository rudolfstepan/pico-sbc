#include "calculator_usb_protocol.h"

#include "calculator_engine.h"
#include "complex_engine.h"
#include "graph_model.h"
#include "logic_engine.h"
#include "number_formats.h"
#include "numerical_analysis.h"
#include "programmer_engine.h"
#include "statistics_engine.h"
#include "unit_engine.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CALCULATOR_FIRMWARE_VERSION "1.8.0"
#define LOGIC_USB_CHUNK_CAPACITY 112u

static void respond(char *response, size_t size, const char *format, ...) {
    if (!response || !size) return;
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(response, size, format, arguments);
    va_end(arguments);
}

static bool word_is(const char *actual, const char *expected) {
    if (!actual || !expected) return false;
    while (*actual && *expected) {
        if (toupper((unsigned char)*actual) !=
            toupper((unsigned char)*expected)) {
            return false;
        }
        actual++;
        expected++;
    }
    return !*actual && !*expected;
}

static char *skip_space(char *cursor) {
    while (*cursor && isspace((unsigned char)*cursor)) cursor++;
    return cursor;
}

static char *remaining_text(char *cursor) {
    cursor = skip_space(cursor);
    size_t length = strlen(cursor);
    while (length && isspace((unsigned char)cursor[length - 1])) {
        cursor[--length] = '\0';
    }
    return cursor;
}

static char *next_word(char **cursor) {
    char *start = skip_space(*cursor);
    if (!*start) {
        *cursor = start;
        return NULL;
    }
    char *end = start;
    while (*end && !isspace((unsigned char)*end)) end++;
    if (*end) *end++ = '\0';
    *cursor = end;
    return start;
}

static bool parse_number(const char *text, double *value) {
    if (!text || !*text || !value) return false;
    errno = 0;
    char *end = NULL;
    double parsed = strtod(text, &end);
    if (end == text || *end || errno == ERANGE || !isfinite(parsed)) {
        return false;
    }
    *value = parsed;
    return true;
}

static bool parse_index(const char *text, size_t *index) {
    if (!text || !*text || !isdigit((unsigned char)*text) || !index) {
        return false;
    }
    errno = 0;
    char *end = NULL;
    unsigned long parsed = strtoul(text, &end, 10);
    if (*end || errno == ERANGE || parsed > (unsigned long)SIZE_MAX) {
        return false;
    }
    *index = (size_t)parsed;
    return true;
}

static bool parse_u64(const char *text, uint64_t *value) {
    if (!text || !*text || !value || *text == '-') return false;
    errno = 0;
    char *end = NULL;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (*end || errno == ERANGE) return false;
    *value = (uint64_t)parsed;
    return true;
}

static bool parse_uint(const char *text, unsigned int *value) {
    size_t parsed = 0;
    if (!parse_index(text, &parsed) || parsed > UINT_MAX) return false;
    *value = (unsigned int)parsed;
    return true;
}

static bool valid_word_bits(unsigned int bits) {
    return bits == 8u || bits == 16u || bits == 32u || bits == 64u;
}

static bool variable_index(const char *word, size_t *index) {
    if (!word || word[1] || toupper((unsigned char)word[0]) < 'A' ||
        toupper((unsigned char)word[0]) > 'F') {
        return false;
    }
    *index = (size_t)(toupper((unsigned char)word[0]) - 'A');
    return true;
}

static bool function_index(const char *word, size_t *index) {
    if (!word || toupper((unsigned char)word[0]) != 'F' ||
        word[1] < '1' || word[1] > '3' || word[2]) {
        return false;
    }
    *index = (size_t)(word[1] - '1');
    return true;
}

static const char *basic_run_state_text(basic_run_state_t state) {
    switch (state) {
        case BASIC_RUN_STOPPED: return "STOPPED";
        case BASIC_RUN_RUNNING: return "RUNNING";
        case BASIC_RUN_INPUT: return "INPUT";
        case BASIC_RUN_FINISHED: return "FINISHED";
        case BASIC_RUN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

static void append_history(calculator_persisted_state_t *state,
                           const char *expression, const char *result_text,
                           double value) {
    if (state->history_count == CALCULATOR_PERSISTENCE_HISTORY_CAPACITY) {
        memmove(&state->history[0], &state->history[1],
                (CALCULATOR_PERSISTENCE_HISTORY_CAPACITY - 1u) *
                sizeof state->history[0]);
        state->history_count--;
    }
    size_t index = state->history_count++;
    snprintf(state->history[index].formula,
             sizeof state->history[index].formula, "%s", expression);
    snprintf(state->history[index].result,
             sizeof state->history[index].result, "%s", result_text);
    state->history[index].value = value;
    state->history_index = index;
}

static void execute_get(calculator_usb_context_t *context, char *cursor,
                        char *response, size_t response_size) {
    calculator_persisted_state_t *state = context->state;
    char *subject = next_word(&cursor);
    if (!subject) {
        respond(response, response_size, "ERR ARGUMENT GET subject");
        return;
    }
    if (word_is(subject, "RESULT")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT RESULT");
        } else {
            respond(response, response_size, "OK RESULT\t%s",
                    state->ans_text[0] ? state->ans_text : "0");
        }
        return;
    }
    if (word_is(subject, "EXPR")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT EXPR");
        } else {
            respond(response, response_size, "OK EXPR\t%s",
                    context->editor->text);
        }
        return;
    }
    if (word_is(subject, "VAR")) {
        char *name = next_word(&cursor);
        size_t index = 0;
        if (!variable_index(name, &index) || *remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT VAR A-F");
        } else {
            respond(response, response_size, "OK VAR\t%s\t%s",
                    calculator_variable_name(index),
                    state->symbols.variable_text[index][0]
                        ? state->symbols.variable_text[index] : "0");
        }
        return;
    }
    if (word_is(subject, "FUNC")) {
        char *name = next_word(&cursor);
        size_t index = 0;
        if (!function_index(name, &index) || *remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT FUNC F1-F3");
        } else {
            respond(response, response_size, "OK FUNC\t%s\t%s",
                    calculator_function_name(index),
                    state->symbols.functions[index]);
        }
        return;
    }
    if (word_is(subject, "HISTORY")) {
        char *index_word = next_word(&cursor);
        if (!index_word) {
            respond(response, response_size, "OK HISTORY\t%u",
                    (unsigned int)state->history_count);
            return;
        }
        size_t index = 0;
        if (!parse_index(index_word, &index) ||
            *remaining_text(cursor) || index >= state->history_count) {
            respond(response, response_size, "ERR INDEX HISTORY");
            return;
        }
        const calculator_persisted_history_entry_t *entry =
            &state->history[index];
        respond(response, response_size, "OK HISTORY\t%u\t%.17g\t%s\t%s",
                (unsigned int)index, entry->value,
                entry->formula, entry->result);
        return;
    }
    if (word_is(subject, "STATS")) {
        char *index_word = next_word(&cursor);
        if (!index_word) {
            respond(response, response_size, "OK STATS\t%u\t%u",
                    state->statistics.two_variable ? 2u : 1u,
                    (unsigned int)state->statistics.count);
            return;
        }
        size_t index = 0;
        if (!parse_index(index_word, &index) ||
            *remaining_text(cursor) || index >= state->statistics.count) {
            respond(response, response_size, "ERR INDEX STATS");
            return;
        }
        if (state->statistics.two_variable) {
            respond(response, response_size, "OK STATS\t%u\t%.17g\t%.17g",
                    (unsigned int)index, state->statistics.x[index],
                    state->statistics.y[index]);
        } else {
            respond(response, response_size, "OK STATS\t%u\t%.17g",
                    (unsigned int)index, state->statistics.x[index]);
        }
        return;
    }
    if (word_is(subject, "ANGLE")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT ANGLE");
        } else {
            respond(response, response_size, "OK ANGLE\t%s",
                    state->degrees ? "DEG" : "RAD");
        }
        return;
    }
    if (word_is(subject, "PRECISION")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT PRECISION");
        } else {
            respond(response, response_size, "OK PRECISION\t%s\t%u",
                    calculator_precision_name(state->precision),
                    calculator_precision_digits(state->precision));
        }
        return;
    }
    if (word_is(subject, "PROGRAMMER")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT PROGRAMMER");
        } else {
            respond(response, response_size,
                    "OK PROGRAMMER_STATE\tvalue=%llu\tbase=%s\tsigned=%u\t"
                    "bit=%u",
                    (unsigned long long)state->programmer_value,
                    programmer_engine_base_name(state->programmer_base),
                    state->programmer_signed ? 1u : 0u,
                    state->programmer_selected_bit);
        }
        return;
    }
    if (word_is(subject, "FORMAT")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT FORMAT");
        } else {
            respond(response, response_size,
                    "OK FORMAT_STATE\tbits=%u\tfraction=%u",
                    state->format_bits, state->fixed_fraction_bits);
        }
        return;
    }
    if (word_is(subject, "MEMORY")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT MEMORY");
        } else {
            respond(response, response_size, "OK MEMORY\t%s",
                    state->memory_text[0] ? state->memory_text : "0");
        }
        return;
    }
    if (word_is(subject, "FAVORITE")) {
        char *index_word = next_word(&cursor);
        size_t index = 0;
        if (!parse_index(index_word, &index) || index < 1u ||
            index > CALCULATOR_FAVORITE_COUNT || *remaining_text(cursor)) {
            respond(response, response_size,
                    "ERR ARGUMENT FAVORITE 1-6");
        } else {
            respond(response, response_size, "OK FAVORITE\t%u\t%s",
                    (unsigned int)index,
                    state->symbols.favorites[index - 1u]);
        }
        return;
    }
    if (word_is(subject, "GRAPH")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT GRAPH");
        } else {
            double x_half = state->graph.x_span * 0.5;
            double y_half = state->graph.y_span * 0.5;
            respond(response, response_size,
                    "OK GRAPH\txmin=%.17g\txmax=%.17g\tymin=%.17g\t"
                    "ymax=%.17g\ttable=%.17g\tstep=%.17g",
                    state->graph.x_center - x_half,
                    state->graph.x_center + x_half,
                    state->graph.y_center - y_half,
                    state->graph.y_center + y_half,
                    state->graph.table_x, state->graph.table_step);
        }
        return;
    }
    if (word_is(subject, "BASIC")) {
        basic_engine_t *engine = context->basic_engine;
        if (!engine) {
            respond(response, response_size, "ERR INTERNAL BASIC");
            return;
        }
        char *item = next_word(&cursor);
        if (!item) {
            respond(response, response_size, "OK BASIC\t%u",
                    (unsigned int)engine->program.count);
            return;
        }
        if (word_is(item, "STATUS")) {
            if (*remaining_text(cursor)) {
                respond(response, response_size,
                        "ERR ARGUMENT GET BASIC STATUS");
                return;
            }
            respond(response, response_size,
                    "OK BASIC_STATUS\tstate=%s\tstatus=%s\tsteps=%u\toutput=%u",
                    basic_run_state_text(engine->state),
                    basic_status_text(engine->status),
                    (unsigned int)engine->instruction_count,
                    (unsigned int)engine->output_count);
            return;
        }
        if (word_is(item, "OUTPUT")) {
            char *index_word = next_word(&cursor);
            if (!index_word) {
                respond(response, response_size, "OK BASIC_OUTPUT\t%u",
                        (unsigned int)engine->output_count);
                return;
            }
            size_t index = 0;
            if (!parse_index(index_word, &index) ||
                *remaining_text(cursor) || index >= engine->output_count) {
                respond(response, response_size, "ERR INDEX BASIC_OUTPUT");
                return;
            }
            respond(response, response_size, "OK BASIC_OUTPUT\t%u\t%s",
                    (unsigned int)index, engine->output[index]);
            return;
        }
        size_t index = 0;
        if (!parse_index(item, &index) || *remaining_text(cursor) ||
            index >= engine->program.count) {
            respond(response, response_size, "ERR INDEX BASIC");
            return;
        }
        respond(response, response_size, "OK BASIC\t%u\t%u\t%s",
                (unsigned int)index,
                (unsigned int)engine->program.lines[index].number,
                engine->program.lines[index].text);
        return;
    }
    respond(response, response_size, "ERR UNKNOWN_SUBJECT %s", subject);
}

static void execute_set(calculator_usb_context_t *context, char *cursor,
                        char *response, size_t response_size,
                        calculator_usb_effect_t *effect) {
    calculator_persisted_state_t *state = context->state;
    char *subject = next_word(&cursor);
    if (!subject) {
        respond(response, response_size, "ERR ARGUMENT SET subject");
        return;
    }
    if (word_is(subject, "EXPR")) {
        char *expression = remaining_text(cursor);
        if (!expression_editor_set(context->editor, expression)) {
            respond(response, response_size, "ERR TOO_LONG EXPR");
            return;
        }
        effect->changed = true;
        respond(response, response_size, "OK EXPR\t%s", expression);
        return;
    }
    if (word_is(subject, "VAR")) {
        char *name = next_word(&cursor);
        char *value_word = next_word(&cursor);
        size_t index = 0;
        double value = 0.0;
        if (!variable_index(name, &index) ||
            !parse_number(value_word, &value) || *remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT VAR A-F value");
            return;
        }
        if (!calculator_symbols_set_variable_precise(
                &state->symbols, index, value, value_word)) {
            respond(response, response_size, "ERR ARGUMENT VAR A-F value");
            return;
        }
        effect->changed = true;
        effect->persistent_changed = true;
        respond(response, response_size, "OK VAR\t%s\t%s",
                calculator_variable_name(index),
                state->symbols.variable_text[index]);
        return;
    }
    if (word_is(subject, "FUNC")) {
        char *name = next_word(&cursor);
        size_t index = 0;
        char *expression = remaining_text(cursor);
        if (!function_index(name, &index)) {
            respond(response, response_size, "ERR ARGUMENT FUNC F1-F3 expr");
            return;
        }
        calculator_symbols_t candidate = state->symbols;
        calculator_symbol_status_t symbol_status =
            calculator_symbols_set_function(&candidate, index, expression);
        size_t invalid_function = CALCULATOR_USER_FUNCTION_COUNT;
        int error_position = 0;
        calc_status_t calc_status = symbol_status == CALCULATOR_SYMBOL_OK
            ? calc_engine_validate_symbols(&candidate, &invalid_function,
                                           &error_position)
            : CALC_PARSE_ERROR;
        if (symbol_status != CALCULATOR_SYMBOL_OK || calc_status != CALC_OK) {
            respond(response, response_size, "ERR INVALID_FUNC %s",
                    symbol_status != CALCULATOR_SYMBOL_OK
                        ? calculator_symbol_status_text(symbol_status)
                        : calc_engine_status_text(calc_status));
            return;
        }
        state->symbols = candidate;
        graph_model_set_function(&state->graph, index,
                                 state->symbols.functions[index]);
        effect->changed = true;
        effect->persistent_changed = true;
        respond(response, response_size, "OK FUNC\t%s\t%s",
                calculator_function_name(index), expression);
        return;
    }
    if (word_is(subject, "ANGLE")) {
        char *mode = next_word(&cursor);
        if (!mode || *remaining_text(cursor) ||
            (!word_is(mode, "DEG") && !word_is(mode, "RAD"))) {
            respond(response, response_size, "ERR ARGUMENT ANGLE DEG|RAD");
            return;
        }
        state->degrees = word_is(mode, "DEG");
        calc_engine_set_degrees(state->degrees);
        effect->changed = true;
        effect->persistent_changed = true;
        respond(response, response_size, "OK ANGLE\t%s",
                state->degrees ? "DEG" : "RAD");
        return;
    }
    if (word_is(subject, "PRECISION")) {
        char *mode = next_word(&cursor);
        calculator_precision_t precision;
        if (!mode || *remaining_text(cursor) ||
            !calculator_precision_parse(mode, &precision)) {
            respond(response, response_size,
                    "ERR ARGUMENT PRECISION NORMAL|HIGH|ULTRA");
            return;
        }
        state->precision = precision;
        calc_engine_set_precision(precision);
        effect->changed = true;
        effect->persistent_changed = true;
        respond(response, response_size, "OK PRECISION\t%s\t%u",
                calculator_precision_name(precision),
                calculator_precision_digits(precision));
        return;
    }
    if (word_is(subject, "MEMORY")) {
        char *value_word = next_word(&cursor);
        double value = 0.0;
        if (!parse_number(value_word, &value) || *remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT MEMORY value");
            return;
        }
        state->memory_value = value;
        snprintf(state->memory_text, sizeof state->memory_text, "%s",
                 value_word);
        effect->changed = true;
        effect->persistent_changed = true;
        respond(response, response_size, "OK MEMORY\t%s", state->memory_text);
        return;
    }
    if (word_is(subject, "PROGRAMMER")) {
        char *value_word = next_word(&cursor);
        char *base_word = next_word(&cursor);
        char *signed_word = next_word(&cursor);
        char *bit_word = next_word(&cursor);
        uint64_t value = 0;
        size_t selected_bit = 0;
        bool base_valid = base_word && (word_is(base_word, "BIN") ||
            word_is(base_word, "DEC") || word_is(base_word, "HEX"));
        if (!parse_u64(value_word, &value) || !base_valid ||
            (!word_is(signed_word, "0") && !word_is(signed_word, "1")) ||
            !parse_index(bit_word, &selected_bit) ||
            selected_bit >= state->format_bits || *remaining_text(cursor)) {
            respond(response, response_size,
                    "ERR ARGUMENT PROGRAMMER value BIN|DEC|HEX 0|1 bit");
            return;
        }
        state->programmer_value = number_format_apply_width(
            value, state->format_bits);
        state->programmer_base = word_is(base_word, "BIN")
            ? PROGRAMMER_BIN : (word_is(base_word, "HEX")
                ? PROGRAMMER_HEX : PROGRAMMER_DEC);
        state->programmer_signed = word_is(signed_word, "1");
        state->programmer_selected_bit = (unsigned int)selected_bit;
        effect->changed = true;
        effect->persistent_changed = true;
        respond(response, response_size,
                "OK PROGRAMMER_STATE\tvalue=%llu\tbase=%s\tsigned=%u\tbit=%u",
                (unsigned long long)state->programmer_value,
                programmer_engine_base_name(state->programmer_base),
                state->programmer_signed ? 1u : 0u,
                state->programmer_selected_bit);
        return;
    }
    if (word_is(subject, "FORMAT")) {
        char *bits_word = next_word(&cursor);
        char *fraction_word = next_word(&cursor);
        unsigned int bits = 0;
        unsigned int fraction = 0;
        if (!parse_uint(bits_word, &bits) || !valid_word_bits(bits) ||
            !parse_uint(fraction_word, &fraction) || fraction >= bits ||
            *remaining_text(cursor)) {
            respond(response, response_size,
                    "ERR ARGUMENT FORMAT bits fraction");
            return;
        }
        state->format_bits = bits;
        state->fixed_fraction_bits = fraction;
        state->programmer_value = number_format_apply_width(
            state->programmer_value, bits);
        if (state->programmer_selected_bit >= bits) {
            state->programmer_selected_bit = bits - 1u;
        }
        effect->changed = true;
        effect->persistent_changed = true;
        respond(response, response_size,
                "OK FORMAT_STATE\tbits=%u\tfraction=%u", bits, fraction);
        return;
    }
    if (word_is(subject, "FAVORITE")) {
        char *index_word = next_word(&cursor);
        char *token = remaining_text(cursor);
        size_t index = 0;
        if (!parse_index(index_word, &index) || index < 1u ||
            index > CALCULATOR_FAVORITE_COUNT || !*token) {
            respond(response, response_size,
                    "ERR ARGUMENT FAVORITE 1-6 token");
            return;
        }
        calculator_symbol_status_t status = calculator_symbols_set_favorite(
            &state->symbols, index - 1u, token);
        if (status != CALCULATOR_SYMBOL_OK) {
            respond(response, response_size, "ERR FAVORITE %s",
                    calculator_symbol_status_text(status));
            return;
        }
        effect->changed = true;
        effect->persistent_changed = true;
        respond(response, response_size, "OK FAVORITE\t%u\t%s",
                (unsigned int)index, token);
        return;
    }
    if (word_is(subject, "GRAPH")) {
        char *x_min_word = next_word(&cursor);
        char *x_max_word = next_word(&cursor);
        char *y_min_word = next_word(&cursor);
        char *y_max_word = next_word(&cursor);
        double x_min = 0.0;
        double x_max = 0.0;
        double y_min = 0.0;
        double y_max = 0.0;
        if (!parse_number(x_min_word, &x_min) ||
            !parse_number(x_max_word, &x_max) ||
            !parse_number(y_min_word, &y_min) ||
            !parse_number(y_max_word, &y_max) || *remaining_text(cursor) ||
            !graph_model_set_range(&state->graph, x_min, x_max,
                                   y_min, y_max)) {
            respond(response, response_size,
                    "ERR ARGUMENT GRAPH xmin xmax ymin ymax");
            return;
        }
        effect->changed = true;
        effect->persistent_changed = true;
        respond(response, response_size,
                "OK GRAPH\t%.17g\t%.17g\t%.17g\t%.17g",
                x_min, x_max, y_min, y_max);
        return;
    }
    respond(response, response_size, "ERR UNKNOWN_SUBJECT %s", subject);
}

static void execute_eval(calculator_usb_context_t *context, char *cursor,
                         char *response, size_t response_size,
                         calculator_usb_effect_t *effect) {
    char *argument = remaining_text(cursor);
    const char *expression = *argument ? argument : context->editor->text;
    if (!*expression) {
        respond(response, response_size, "ERR EMPTY EXPR");
        return;
    }
    if (strlen(expression) >= EXPRESSION_EDITOR_CAPACITY) {
        respond(response, response_size, "ERR TOO_LONG EXPR");
        return;
    }
    calc_engine_set_degrees(context->state->degrees);
    calc_engine_set_precision(context->state->precision);
    calculator_result_t result;
    int error_position = 0;
    calc_status_t status = calc_engine_evaluate_precise_symbols(
        expression, context->state->ans, context->state->ans_text,
        &context->state->symbols, &result, &error_position);
    if (status != CALC_OK) {
        if (status == CALC_PARSE_ERROR && error_position > 0) {
            respond(response, response_size, "ERR PARSE %d", error_position);
        } else {
            respond(response, response_size, "ERR EVAL %s",
                    calc_engine_status_text(status));
        }
        return;
    }
    expression_editor_set(context->editor, expression);
    context->state->ans = result.value;
    snprintf(context->state->ans_text, sizeof context->state->ans_text,
             "%s", result.text);
    append_history(context->state, expression, result.text, result.value);
    effect->changed = true;
    effect->evaluated = true;
    effect->persistent_changed = true;
    respond(response, response_size, "OK RESULT\t%s", result.text);
}

static void execute_stat(calculator_usb_context_t *context, char *cursor,
                         char *response, size_t response_size,
                         calculator_usb_effect_t *effect) {
    char *action = next_word(&cursor);
    if (!action) {
        respond(response, response_size, "ERR ARGUMENT STAT action");
        return;
    }
    statistics_dataset_t *statistics = &context->state->statistics;
    if (word_is(action, "MODE")) {
        char *mode = next_word(&cursor);
        if (!mode || *remaining_text(cursor) ||
            (!word_is(mode, "1") && !word_is(mode, "2"))) {
            respond(response, response_size, "ERR ARGUMENT STAT MODE 1|2");
            return;
        }
        statistics_engine_set_two_variable(statistics, word_is(mode, "2"));
        effect->changed = true;
        effect->persistent_changed = true;
        effect->statistics_changed = true;
        respond(response, response_size, "OK STATS\t%u\t%u",
                statistics->two_variable ? 2u : 1u,
                (unsigned int)statistics->count);
        return;
    }
    if (word_is(action, "CLEAR")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT STAT CLEAR");
            return;
        }
        statistics_engine_clear(statistics);
        effect->changed = true;
        effect->persistent_changed = true;
        effect->statistics_changed = true;
        respond(response, response_size, "OK STATS\t%u\t0",
                statistics->two_variable ? 2u : 1u);
        return;
    }
    if (word_is(action, "ADD")) {
        char *x_word = next_word(&cursor);
        char *y_word = next_word(&cursor);
        double x = 0.0;
        double y = 0.0;
        bool valid = parse_number(x_word, &x) &&
            (statistics->two_variable
                ? parse_number(y_word, &y)
                : y_word == NULL) &&
            !*remaining_text(cursor);
        if (!valid) {
            respond(response, response_size,
                    statistics->two_variable
                        ? "ERR ARGUMENT STAT ADD x y"
                        : "ERR ARGUMENT STAT ADD x");
            return;
        }
        statistics_status_t status = statistics_engine_add(statistics, x, y);
        if (status != STATISTICS_STATUS_OK) {
            respond(response, response_size, "ERR STATS %s",
                    statistics_engine_status_text(status));
            return;
        }
        effect->changed = true;
        effect->persistent_changed = true;
        effect->statistics_changed = true;
        respond(response, response_size, "OK STATS\t%u\t%u",
                statistics->two_variable ? 2u : 1u,
                (unsigned int)statistics->count);
        return;
    }
    if (word_is(action, "SUMMARY")) {
        char *axis = next_word(&cursor);
        bool use_y = axis && word_is(axis, "Y");
        if (!axis || (!word_is(axis, "X") && !use_y) ||
            *remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT STAT SUMMARY X|Y");
            return;
        }
        statistics_summary_t summary;
        statistics_status_t status = statistics_engine_summary(
            statistics, use_y, &summary);
        if (status != STATISTICS_STATUS_OK) {
            respond(response, response_size, "ERR STATS %s",
                    statistics_engine_status_text(status));
            return;
        }
        respond(response, response_size,
                "OK STATS_SUMMARY\taxis=%s\tcount=%u\tmean=%.17g\t"
                "median=%.17g\tmin=%.17g\tmax=%.17g\tpopstd=%.17g\t"
                "samplestd=%.17g",
                use_y ? "Y" : "X", (unsigned int)summary.count,
                summary.mean, summary.median, summary.minimum, summary.maximum,
                summary.population_stddev, summary.sample_stddev);
        return;
    }
    if (word_is(action, "REGRESSION")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT STAT REGRESSION");
            return;
        }
        statistics_regression_t regression;
        statistics_status_t status = statistics_engine_regression(
            statistics, &regression);
        if (status != STATISTICS_STATUS_OK) {
            respond(response, response_size, "ERR STATS %s",
                    statistics_engine_status_text(status));
            return;
        }
        respond(response, response_size,
                "OK STATS_REGRESSION\tslope=%.17g\tintercept=%.17g\t"
                "correlation=%.17g",
                regression.slope, regression.intercept,
                regression.correlation);
        return;
    }
    if (word_is(action, "HISTOGRAM")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT STAT HISTOGRAM");
            return;
        }
        size_t counts[STATISTICS_HISTOGRAM_BINS];
        double minimum = 0.0;
        double maximum = 0.0;
        statistics_status_t status = statistics_engine_histogram(
            statistics, counts, &minimum, &maximum);
        if (status != STATISTICS_STATUS_OK) {
            respond(response, response_size, "ERR STATS %s",
                    statistics_engine_status_text(status));
            return;
        }
        respond(response, response_size,
                "OK STATS_HISTOGRAM\tmin=%.17g\tmax=%.17g\t"
                "bins=%u,%u,%u,%u,%u,%u,%u,%u",
                minimum, maximum,
                (unsigned int)counts[0], (unsigned int)counts[1],
                (unsigned int)counts[2], (unsigned int)counts[3],
                (unsigned int)counts[4], (unsigned int)counts[5],
                (unsigned int)counts[6], (unsigned int)counts[7]);
        return;
    }
    respond(response, response_size, "ERR UNKNOWN_ACTION %s", action);
}

static void programmer_response(const programmer_engine_t *engine,
                                char *response, size_t response_size) {
    char binary[65];
    char signed_text[32];
    uint64_t value = number_format_apply_width(engine->value,
                                                engine->word_bits);
    for (unsigned int i = 0; i < engine->word_bits; ++i) {
        unsigned int bit = engine->word_bits - i - 1u;
        binary[i] = (value & (UINT64_C(1) << bit)) ? '1' : '0';
    }
    binary[engine->word_bits] = '\0';
    number_format_signed_text(value, engine->word_bits,
                              signed_text, sizeof signed_text);
    respond(response, response_size,
            "OK PROGRAMMER\tvalue=%llu\tsigned=%s\thex=%0*llX\tbin=%s\t"
            "carry=%u\toverflow=%u",
            (unsigned long long)value, signed_text,
            (int)(engine->word_bits / 4u), (unsigned long long)value,
            binary, engine->carry ? 1u : 0u, engine->overflow ? 1u : 0u);
}

static void execute_programmer(char *cursor, char *response,
                               size_t response_size) {
    char *action = next_word(&cursor);
    char *value_word = next_word(&cursor);
    char *bits_word = next_word(&cursor);
    char *operand_word = next_word(&cursor);
    uint64_t value = 0;
    uint64_t operand = 0;
    unsigned int bits = 0;
    if (!action || !parse_u64(value_word, &value) ||
        !parse_uint(bits_word, &bits) || !valid_word_bits(bits) ||
        *remaining_text(cursor)) {
        respond(response, response_size,
                "ERR ARGUMENT PROGRAMMER action value bits [operand]");
        return;
    }
    bool needs_operand = word_is(action, "AND") || word_is(action, "OR") ||
        word_is(action, "XOR") || word_is(action, "BSET") ||
        word_is(action, "BCLEAR") || word_is(action, "BTOGGLE");
    if ((needs_operand && !parse_u64(operand_word, &operand)) ||
        (!needs_operand && operand_word)) {
        respond(response, response_size,
                "ERR ARGUMENT PROGRAMMER operand");
        return;
    }

    programmer_engine_t engine;
    programmer_engine_init(&engine);
    programmer_engine_set_word_bits(&engine, bits);
    programmer_engine_set_value(&engine, value);
    if (word_is(action, "VIEW")) {
        /* Value is already loaded. */
    } else if (word_is(action, "AND") || word_is(action, "OR") ||
               word_is(action, "XOR")) {
        programmer_binary_op_t operation = word_is(action, "AND")
            ? PROGRAMMER_OP_AND : (word_is(action, "OR")
                ? PROGRAMMER_OP_OR : PROGRAMMER_OP_XOR);
        programmer_engine_set_binary_op(&engine, operation);
        programmer_engine_set_value(&engine, operand);
        programmer_engine_equals(&engine);
    } else if (word_is(action, "NOT")) {
        programmer_engine_not(&engine);
    } else if (word_is(action, "NEG")) {
        programmer_engine_negate(&engine);
    } else if (word_is(action, "SHL")) {
        programmer_engine_shift_left(&engine);
    } else if (word_is(action, "SHR")) {
        programmer_engine_shift_right(&engine);
    } else if (word_is(action, "ROL")) {
        programmer_engine_rotate_left(&engine);
    } else if (word_is(action, "ROR")) {
        programmer_engine_rotate_right(&engine);
    } else if (word_is(action, "SWAP")) {
        programmer_engine_swap_endian(&engine);
    } else if (word_is(action, "INC")) {
        programmer_engine_increment(&engine);
    } else if (word_is(action, "DEC")) {
        programmer_engine_decrement(&engine);
    } else if (word_is(action, "BSET") || word_is(action, "BCLEAR") ||
               word_is(action, "BTOGGLE")) {
        if (operand >= bits) {
            respond(response, response_size, "ERR INDEX BIT");
            return;
        }
        engine.selected_bit = (unsigned int)operand;
        if (word_is(action, "BSET")) programmer_engine_set_selected_bit(&engine);
        else if (word_is(action, "BCLEAR")) {
            programmer_engine_clear_selected_bit(&engine);
        } else {
            programmer_engine_toggle_selected_bit(&engine);
        }
    } else {
        respond(response, response_size, "ERR UNKNOWN_ACTION %s", action);
        return;
    }
    programmer_response(&engine, response, response_size);
}

static void execute_format(char *cursor, char *response,
                           size_t response_size) {
    char *value_word = next_word(&cursor);
    char *bits_word = next_word(&cursor);
    char *fraction_word = next_word(&cursor);
    uint64_t value = 0;
    unsigned int bits = 0;
    unsigned int fraction = 0;
    if (!parse_u64(value_word, &value) || !parse_uint(bits_word, &bits) ||
        !valid_word_bits(bits) || !parse_uint(fraction_word, &fraction) ||
        fraction >= bits || *remaining_text(cursor)) {
        respond(response, response_size,
                "ERR ARGUMENT FORMAT value bits fraction");
        return;
    }
    value = number_format_apply_width(value, bits);
    char signed_text[32];
    number_format_signed_text(value, bits, signed_text, sizeof signed_text);
    double fixed = number_format_fixed_value(value, bits, fraction);
    respond(response, response_size,
            "OK FORMAT\tunsigned=%llu\tsigned=%s\tfixed=%.17g\t"
            "float32=%08lX\tfloat64=%016llX",
            (unsigned long long)value, signed_text, fixed,
            (unsigned long)number_format_float32_bits(fixed),
            (unsigned long long)number_format_float64_bits(fixed));
}

static void execute_ieee(char *cursor, char *response,
                         size_t response_size) {
    char *width_word = next_word(&cursor);
    char *raw_word = next_word(&cursor);
    unsigned int width = 0;
    uint64_t raw = 0;
    if (!parse_uint(width_word, &width) ||
        (width != 32u && width != 64u) || !parse_u64(raw_word, &raw) ||
        *remaining_text(cursor) || (width == 32u && raw > UINT32_MAX)) {
        respond(response, response_size, "ERR ARGUMENT IEEE 32|64 raw");
        return;
    }
    number_format_ieee_t inspected = width == 32u
        ? number_format_inspect_float32((uint32_t)raw)
        : number_format_inspect_float64(raw);
    double value = width == 32u
        ? number_format_bits_float32((uint32_t)raw)
        : number_format_bits_float64(raw);
    respond(response, response_size,
            "OK IEEE\twidth=%u\tsign=%u\trawexp=%u\texponent=%d\t"
            "mantissa=%llu\tclass=%s\tvalue=%.17g",
            width, inspected.sign ? 1u : 0u,
            (unsigned int)inspected.raw_exponent,
            inspected.unbiased_exponent,
            (unsigned long long)inspected.mantissa,
            number_format_ieee_class_name(inspected.classification), value);
}

static bool compile_logic(const char *expression, logic_program_t *program,
                          char *response, size_t response_size) {
    int error_position = 0;
    logic_status_t status = logic_engine_compile(expression, program,
                                                  &error_position);
    if (status == LOGIC_STATUS_OK) return true;
    if (status == LOGIC_STATUS_SYNTAX && error_position > 0) {
        respond(response, response_size, "ERR LOGIC SYNTAX %d",
                error_position);
    } else {
        respond(response, response_size, "ERR LOGIC %s",
                logic_engine_status_text(status));
    }
    return false;
}

static void execute_logic(char *cursor, char *response,
                          size_t response_size) {
    char *action = next_word(&cursor);
    if (!action) {
        respond(response, response_size, "ERR ARGUMENT LOGIC action");
        return;
    }
    logic_program_t program;
    if (word_is(action, "INFO")) {
        char *expression = remaining_text(cursor);
        if (!compile_logic(expression, &program, response, response_size)) return;
        respond(response, response_size, "OK LOGIC_INFO\tmask=%u\tvars=%u\trows=%u",
                (unsigned int)program.variable_mask,
                (unsigned int)logic_engine_variable_count(&program),
                (unsigned int)logic_engine_truth_row_count(&program));
        return;
    }
    if (word_is(action, "EVAL") || word_is(action, "ROW")) {
        char *number_word = next_word(&cursor);
        size_t number = 0;
        char *expression = remaining_text(cursor);
        if (!parse_index(number_word, &number) ||
            !compile_logic(expression, &program, response, response_size)) return;
        uint8_t assignment;
        if (word_is(action, "ROW")) {
            if (number >= logic_engine_truth_row_count(&program)) {
                respond(response, response_size, "ERR INDEX LOGIC_ROW");
                return;
            }
            assignment = logic_engine_assignment_for_row(&program, number);
        } else {
            if (number > UINT8_MAX) {
                respond(response, response_size, "ERR ARGUMENT LOGIC assignment");
                return;
            }
            assignment = (uint8_t)number;
        }
        respond(response, response_size,
                "OK LOGIC_VALUE\tassignment=%u\tvalue=%u\tmask=%u",
                (unsigned int)assignment,
                logic_engine_evaluate(&program, assignment) ? 1u : 0u,
                (unsigned int)program.variable_mask);
        return;
    }
    if (word_is(action, "FORM")) {
        char *kind = next_word(&cursor);
        char *style = next_word(&cursor);
        char *offset_word = next_word(&cursor);
        size_t offset = 0;
        char *expression = remaining_text(cursor);
        if ((!word_is(kind, "DNF") && !word_is(kind, "KNF")) ||
            (!word_is(style, "SIMPLE") && !word_is(style, "CANONICAL")) ||
            !parse_index(offset_word, &offset) ||
            !compile_logic(expression, &program, response, response_size)) return;
        static char form[LOGIC_FORM_CAPACITY];
        logic_status_t status = word_is(style, "SIMPLE")
            ? logic_engine_format_simplified(&program, word_is(kind, "DNF"),
                                             form, sizeof form)
            : logic_engine_format_canonical(&program, word_is(kind, "DNF"),
                                            form, sizeof form);
        if (status != LOGIC_STATUS_OK) {
            respond(response, response_size, "ERR LOGIC %s",
                    logic_engine_status_text(status));
            return;
        }
        size_t total = strlen(form);
        if (offset > total) {
            respond(response, response_size, "ERR INDEX LOGIC_FORM");
            return;
        }
        char chunk[LOGIC_USB_CHUNK_CAPACITY + 1u];
        size_t count = total - offset;
        if (count > LOGIC_USB_CHUNK_CAPACITY) count = LOGIC_USB_CHUNK_CAPACITY;
        memcpy(chunk, form + offset, count);
        chunk[count] = '\0';
        respond(response, response_size,
                "OK LOGIC_FORM\ttotal=%u\toffset=%u\tdata=%s",
                (unsigned int)total, (unsigned int)offset, chunk);
        return;
    }
    respond(response, response_size, "ERR UNKNOWN_ACTION %s", action);
}

static void execute_unit(char *cursor, char *response,
                         size_t response_size) {
    char *action = next_word(&cursor);
    if (!action) {
        respond(response, response_size, "ERR ARGUMENT UNIT action");
        return;
    }
    size_t category = 0;
    if (word_is(action, "CATEGORY")) {
        char *category_word = next_word(&cursor);
        if (!parse_index(category_word, &category) ||
            category >= UNIT_CATEGORY_COUNT || *remaining_text(cursor)) {
            respond(response, response_size, "ERR INDEX UNIT_CATEGORY");
            return;
        }
        respond(response, response_size,
                "OK UNIT_CATEGORY\tindex=%u\tname=%s\tcount=%u",
                (unsigned int)category,
                unit_engine_category_name((unit_category_t)category),
                (unsigned int)unit_engine_unit_count((unit_category_t)category));
        return;
    }
    char *category_word = next_word(&cursor);
    if (!parse_index(category_word, &category) ||
        category >= UNIT_CATEGORY_COUNT) {
        respond(response, response_size, "ERR INDEX UNIT_CATEGORY");
        return;
    }
    if (word_is(action, "ITEM")) {
        char *index_word = next_word(&cursor);
        size_t index = 0;
        if (!parse_index(index_word, &index) || *remaining_text(cursor)) {
            respond(response, response_size, "ERR INDEX UNIT");
            return;
        }
        const unit_definition_t *unit = unit_engine_unit(
            (unit_category_t)category, index);
        if (!unit) {
            respond(response, response_size, "ERR INDEX UNIT");
            return;
        }
        respond(response, response_size,
                "OK UNIT\tindex=%u\tname=%s\tsymbol=%s",
                (unsigned int)index, unit->name, unit->symbol);
        return;
    }
    if (word_is(action, "CONVERT")) {
        char *from_word = next_word(&cursor);
        char *to_word = next_word(&cursor);
        char *value_word = next_word(&cursor);
        size_t from_index = 0;
        size_t to_index = 0;
        double value = 0.0;
        if (!parse_index(from_word, &from_index) ||
            !parse_index(to_word, &to_index) ||
            !parse_number(value_word, &value) || *remaining_text(cursor)) {
            respond(response, response_size,
                    "ERR ARGUMENT UNIT CONVERT category from to value");
            return;
        }
        const unit_definition_t *from = unit_engine_unit(
            (unit_category_t)category, from_index);
        const unit_definition_t *to = unit_engine_unit(
            (unit_category_t)category, to_index);
        double result = 0.0;
        unit_status_t status = unit_engine_convert(value, from, to, &result);
        if (status != UNIT_STATUS_OK) {
            respond(response, response_size, "ERR UNIT %s",
                    unit_engine_status_text(status));
            return;
        }
        respond(response, response_size,
                "OK UNIT_RESULT\tvalue=%.17g\tfrom=%s\tto=%s",
                result, from->symbol, to->symbol);
        return;
    }
    respond(response, response_size, "ERR UNKNOWN_ACTION %s", action);
}

static void execute_constant(char *cursor, char *response,
                             size_t response_size) {
    char *item = next_word(&cursor);
    if (!item || *remaining_text(cursor)) {
        respond(response, response_size, "ERR ARGUMENT CONSTANT COUNT|index");
        return;
    }
    if (word_is(item, "COUNT")) {
        respond(response, response_size, "OK CONSTANTS\t%u",
                (unsigned int)unit_engine_constant_count());
        return;
    }
    size_t index = 0;
    if (!parse_index(item, &index)) {
        respond(response, response_size, "ERR INDEX CONSTANT");
        return;
    }
    const physical_constant_t *constant = unit_engine_constant(index);
    if (!constant) {
        respond(response, response_size, "ERR INDEX CONSTANT");
        return;
    }
    respond(response, response_size,
            "OK CONSTANT\tindex=%u\tname=%s\tsymbol=%s\tvalue=%.17g\t"
            "unit=%s\tsource=%s",
            (unsigned int)index, constant->name, constant->symbol,
            constant->value, constant->unit, constant->source);
}

static void execute_complex(char *cursor, char *response,
                            size_t response_size) {
    char *mode = next_word(&cursor);
    char *expression = remaining_text(cursor);
    if ((!word_is(mode, "DEG") && !word_is(mode, "RAD")) || !*expression) {
        respond(response, response_size,
                "ERR ARGUMENT COMPLEX DEG|RAD expression");
        return;
    }
    complex_value_t result;
    int error_position = 0;
    complex_status_t status = complex_engine_evaluate(
        expression, word_is(mode, "DEG"), &result, &error_position);
    if (status != COMPLEX_STATUS_OK) {
        if (status == COMPLEX_STATUS_SYNTAX && error_position > 0) {
            respond(response, response_size, "ERR COMPLEX SYNTAX %d",
                    error_position);
        } else {
            respond(response, response_size, "ERR COMPLEX %s",
                    complex_engine_status_text(status));
        }
        return;
    }
    char cartesian[64];
    char polar[64];
    complex_engine_format(result, false, word_is(mode, "DEG"),
                          cartesian, sizeof cartesian);
    complex_engine_format(result, true, word_is(mode, "DEG"),
                          polar, sizeof polar);
    respond(response, response_size,
            "OK COMPLEX\treal=%.17g\timag=%.17g\tcart=%s\tpolar=%s",
            result.real, result.imag, cartesian, polar);
}

typedef struct {
    calc_compiled_t *compiled;
} usb_graph_function_t;

static bool usb_graph_evaluate(void *context, double x, double *y) {
    usb_graph_function_t *function = context;
    return function && function->compiled &&
        calc_engine_evaluate_x(function->compiled, x, y);
}

static bool compile_graph_function(calculator_persisted_state_t *state,
                                   size_t index, usb_graph_function_t *function,
                                   char *response, size_t response_size) {
    if (index >= CALCULATOR_USER_FUNCTION_COUNT ||
        !state->symbols.functions[index][0]) {
        respond(response, response_size, "ERR GRAPH EMPTY_FUNCTION");
        return false;
    }
    calc_engine_set_degrees(state->degrees);
    int error_position = 0;
    function->compiled = calc_engine_compile_x_symbols(
        state->symbols.functions[index], state->ans, &state->symbols,
        &error_position);
    if (!function->compiled) {
        respond(response, response_size, "ERR GRAPH PARSE %d", error_position);
        return false;
    }
    return true;
}

static void execute_graph(calculator_usb_context_t *context, char *cursor,
                          char *response, size_t response_size) {
    char *action = next_word(&cursor);
    char *first_word = next_word(&cursor);
    size_t first = 0;
    if (!action || !function_index(first_word, &first)) {
        respond(response, response_size, "ERR ARGUMENT GRAPH action F1-F3");
        return;
    }
    usb_graph_function_t first_function = {0};
    if (!compile_graph_function(context->state, first, &first_function,
                                response, response_size)) return;
    numerical_options_t options = numerical_default_options();
    if (word_is(action, "EVAL")) {
        char *x_word = next_word(&cursor);
        double x = 0.0;
        double y = 0.0;
        if (!parse_number(x_word, &x) || *remaining_text(cursor) ||
            !usb_graph_evaluate(&first_function, x, &y)) {
            respond(response, response_size, "ERR GRAPH EVAL");
        } else {
            respond(response, response_size,
                    "OK GRAPH_VALUE\tfunction=F%u\tx=%.17g\ty=%.17g",
                    (unsigned int)first + 1u, x, y);
        }
        calc_engine_free(first_function.compiled);
        return;
    }
    numerical_result_t result = {.status = CALC_PARSE_ERROR};
    if (word_is(action, "ROOT") || word_is(action, "INTEGR") ||
        word_is(action, "EXTREMA")) {
        char *left_word = next_word(&cursor);
        char *right_word = next_word(&cursor);
        double left = 0.0;
        double right = 0.0;
        if (!parse_number(left_word, &left) ||
            !parse_number(right_word, &right) || *remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT GRAPH interval");
            calc_engine_free(first_function.compiled);
            return;
        } else if (word_is(action, "ROOT")) {
            result = numerical_root_interval(usb_graph_evaluate,
                                             &first_function, left, right,
                                             options);
        } else if (word_is(action, "INTEGR")) {
            result = numerical_integral(usb_graph_evaluate,
                                        &first_function, left, right, options);
        } else {
            numerical_extremum_t extrema[12];
            calc_status_t status = CALC_OK;
            size_t count = numerical_find_extrema(
                usb_graph_evaluate, &first_function, left, right, options,
                extrema, sizeof extrema / sizeof extrema[0], &status);
            if (status != CALC_OK) {
                respond(response, response_size, "ERR GRAPH %s",
                        calc_engine_status_text(status));
            } else {
                size_t minimum = count;
                size_t maximum = count;
                for (size_t i = 0; i < count; ++i) {
                    if (extrema[i].kind == NUMERICAL_MINIMUM && minimum == count) {
                        minimum = i;
                    }
                    if (extrema[i].kind == NUMERICAL_MAXIMUM && maximum == count) {
                        maximum = i;
                    }
                }
                respond(response, response_size,
                        "OK GRAPH_EXTREMA\tcount=%u\tminx=%.17g\tminy=%.17g\t"
                        "maxx=%.17g\tmaxy=%.17g",
                        (unsigned int)count,
                        minimum < count ? extrema[minimum].x : NAN,
                        minimum < count ? extrema[minimum].value : NAN,
                        maximum < count ? extrema[maximum].x : NAN,
                        maximum < count ? extrema[maximum].value : NAN);
            }
            calc_engine_free(first_function.compiled);
            return;
        }
    } else if (word_is(action, "DERIV")) {
        char *x_word = next_word(&cursor);
        double x = 0.0;
        if (!parse_number(x_word, &x) || *remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT GRAPH DERIV F x");
            calc_engine_free(first_function.compiled);
            return;
        } else {
            result = numerical_derivative(usb_graph_evaluate,
                                          &first_function, x, options);
        }
    } else if (word_is(action, "XING")) {
        char *second_word = next_word(&cursor);
        char *left_word = next_word(&cursor);
        char *right_word = next_word(&cursor);
        size_t second = 0;
        double left = 0.0;
        double right = 0.0;
        usb_graph_function_t second_function = {0};
        if (!function_index(second_word, &second) ||
            !parse_number(left_word, &left) ||
            !parse_number(right_word, &right) || *remaining_text(cursor)) {
            respond(response, response_size,
                    "ERR ARGUMENT GRAPH XING F1 F2 left right");
            calc_engine_free(first_function.compiled);
            return;
        }
        if (!compile_graph_function(context->state, second, &second_function,
                                    response, response_size)) {
            calc_engine_free(first_function.compiled);
            return;
        }
        result = numerical_intersection_interval(
            usb_graph_evaluate, &first_function,
            usb_graph_evaluate, &second_function,
            left, right, options);
        calc_engine_free(second_function.compiled);
    } else {
        respond(response, response_size, "ERR UNKNOWN_ACTION %s", action);
        calc_engine_free(first_function.compiled);
        return;
    }
    calc_engine_free(first_function.compiled);
    if (result.status != CALC_OK) {
        respond(response, response_size, "ERR GRAPH %s",
                calc_engine_status_text(result.status));
    } else {
        respond(response, response_size,
                "OK GRAPH_ANALYSIS\taction=%s\tx=%.17g\tvalue=%.17g\t"
                "iterations=%u",
                action, result.x, result.value, result.iterations);
    }
}

static void execute_module(calculator_usb_context_t *context, char *cursor,
                           char *response, size_t response_size) {
    char *module = next_word(&cursor);
    if (!module) {
        respond(response, response_size, "ERR ARGUMENT MODULE name");
    } else if (word_is(module, "PROGRAMMER")) {
        execute_programmer(cursor, response, response_size);
    } else if (word_is(module, "FORMAT")) {
        execute_format(cursor, response, response_size);
    } else if (word_is(module, "IEEE")) {
        execute_ieee(cursor, response, response_size);
    } else if (word_is(module, "GRAPH")) {
        execute_graph(context, cursor, response, response_size);
    } else if (word_is(module, "LOGIC")) {
        execute_logic(cursor, response, response_size);
    } else if (word_is(module, "UNIT")) {
        execute_unit(cursor, response, response_size);
    } else if (word_is(module, "CONSTANT")) {
        execute_constant(cursor, response, response_size);
    } else if (word_is(module, "COMPLEX")) {
        execute_complex(cursor, response, response_size);
    } else {
        respond(response, response_size, "ERR UNKNOWN_MODULE %s", module);
    }
}

static void execute_basic(calculator_usb_context_t *context, char *cursor,
                          char *response, size_t response_size,
                          calculator_usb_effect_t *effect) {
    basic_engine_t *engine = context->basic_engine;
    if (!engine) {
        respond(response, response_size, "ERR INTERNAL BASIC");
        return;
    }
    char *action = next_word(&cursor);
    if (!action) {
        respond(response, response_size, "ERR ARGUMENT BASIC action");
        return;
    }
    if (word_is(action, "LINE")) {
        char *source = remaining_text(cursor);
        if (!*source) {
            respond(response, response_size, "ERR ARGUMENT BASIC LINE source");
            return;
        }
        basic_status_t status = basic_engine_store_line(engine, source);
        if (status != BASIC_STATUS_OK) {
            respond(response, response_size, "ERR BASIC %s",
                    basic_status_text(status));
            return;
        }
        context->state->basic_program = engine->program;
        effect->changed = true;
        effect->persistent_changed = true;
        effect->basic_program_changed = true;
        respond(response, response_size, "OK BASIC\t%u",
                (unsigned int)engine->program.count);
        return;
    }
    if (word_is(action, "CLEAR")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT BASIC CLEAR");
            return;
        }
        basic_engine_clear_program(engine);
        context->state->basic_program = engine->program;
        effect->changed = true;
        effect->persistent_changed = true;
        effect->basic_program_changed = true;
        respond(response, response_size, "OK BASIC\t0");
        return;
    }
    if (word_is(action, "RUN")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT BASIC RUN");
            return;
        }
        basic_status_t status = basic_engine_run(engine);
        if (status != BASIC_STATUS_OK) {
            respond(response, response_size, "ERR BASIC %s",
                    basic_status_text(status));
            return;
        }
        effect->basic_runtime_changed = true;
        respond(response, response_size, "OK BASIC_RUN\tRUNNING");
        return;
    }
    if (word_is(action, "STOP")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT BASIC STOP");
            return;
        }
        basic_engine_stop(engine);
        effect->basic_runtime_changed = true;
        respond(response, response_size, "OK BASIC_RUN\tSTOPPED");
        return;
    }
    if (word_is(action, "INPUT")) {
        char *input = remaining_text(cursor);
        if (!*input) {
            respond(response, response_size, "ERR ARGUMENT BASIC INPUT value");
            return;
        }
        basic_status_t status = basic_engine_submit_input(engine, input);
        if (status != BASIC_STATUS_OK) {
            respond(response, response_size, "ERR BASIC %s",
                    basic_status_text(status));
            return;
        }
        effect->basic_runtime_changed = true;
        respond(response, response_size, "OK BASIC_INPUT\tRUNNING");
        return;
    }
    respond(response, response_size, "ERR UNKNOWN_ACTION %s", action);
}

void calculator_usb_execute(calculator_usb_context_t *context,
                            const char *line,
                            char *response, size_t response_size,
                            calculator_usb_effect_t *effect) {
    if (effect) memset(effect, 0, sizeof *effect);
    if (!response || !response_size) return;
    response[0] = '\0';
    if (!context || !context->state || !context->editor || !line) {
        respond(response, response_size, "ERR INTERNAL context");
        return;
    }
    size_t length = strlen(line);
    if (length >= CALCULATOR_USB_LINE_CAPACITY) {
        respond(response, response_size, "ERR LINE_TOO_LONG");
        return;
    }
    for (size_t i = 0; i < length; ++i) {
        unsigned char character = (unsigned char)line[i];
        if (character < 0x20u || character > 0x7eu) {
            respond(response, response_size, "ERR INVALID_CHAR");
            return;
        }
    }

    char command_line[CALCULATOR_USB_LINE_CAPACITY];
    snprintf(command_line, sizeof command_line, "%s", line);
    char *cursor = command_line;
    char *command = next_word(&cursor);
    if (!command) {
        respond(response, response_size, "ERR EMPTY");
        return;
    }
    calculator_usb_effect_t local_effect = {0};
    if (word_is(command, "PING")) {
        respond(response, response_size, *remaining_text(cursor)
                ? "ERR ARGUMENT PING" : "OK PONG");
    } else if (word_is(command, "INFO")) {
        respond(response, response_size, *remaining_text(cursor)
                ? "ERR ARGUMENT INFO"
                : "OK INFO\tprotocol=%u\tfirmware=%s\tmodel=scientific-calculator",
                CALCULATOR_USB_PROTOCOL_VERSION, CALCULATOR_FIRMWARE_VERSION);
    } else if (word_is(command, "DIAG")) {
        if (*remaining_text(cursor)) {
            respond(response, response_size, "ERR ARGUMENT DIAG");
        } else {
            respond(response, response_size,
                    "OK DIAG\tpage=%u\tangle=%s\tprecision=%s\thistory=%u"
                    "\tstats=%u\tmode=%u"
                    "\tbasic=%u\tbasic_state=%s",
                    (unsigned int)context->state->page,
                    context->state->degrees ? "DEG" : "RAD",
                    calculator_precision_name(context->state->precision),
                    (unsigned int)context->state->history_count,
                    (unsigned int)context->state->statistics.count,
                    context->state->statistics.two_variable ? 2u : 1u,
                    context->basic_engine
                        ? (unsigned int)context->basic_engine->program.count : 0u,
                    context->basic_engine
                        ? basic_run_state_text(context->basic_engine->state)
                        : "UNKNOWN");
        }
    } else if (word_is(command, "GET")) {
        execute_get(context, cursor, response, response_size);
    } else if (word_is(command, "SET")) {
        execute_set(context, cursor, response, response_size, &local_effect);
    } else if (word_is(command, "EVAL")) {
        execute_eval(context, cursor, response, response_size, &local_effect);
    } else if (word_is(command, "STAT")) {
        execute_stat(context, cursor, response, response_size, &local_effect);
    } else if (word_is(command, "MODULE")) {
        execute_module(context, cursor, response, response_size);
    } else if (word_is(command, "BASIC")) {
        execute_basic(context, cursor, response, response_size, &local_effect);
    } else {
        respond(response, response_size, "ERR UNKNOWN_COMMAND %s", command);
    }
    if (effect) *effect = local_effect;
}

void calculator_usb_line_reader_init(calculator_usb_line_reader_t *reader) {
    if (reader) memset(reader, 0, sizeof *reader);
}

calculator_usb_line_status_t calculator_usb_line_reader_feed(
    calculator_usb_line_reader_t *reader, int character) {
    if (!reader) return CALCULATOR_USB_LINE_INVALID;
    if (character == '\r') return CALCULATOR_USB_LINE_NONE;
    if (character == '\n') {
        calculator_usb_line_status_t status = reader->overflow
            ? CALCULATOR_USB_LINE_TOO_LONG
            : (reader->invalid ? CALCULATOR_USB_LINE_INVALID
                               : CALCULATOR_USB_LINE_READY);
        reader->line[reader->length] = '\0';
        reader->length = 0;
        reader->overflow = false;
        reader->invalid = false;
        return status;
    }
    if (character == '\b' || character == 0x7f) {
        if (!reader->overflow && !reader->invalid && reader->length) {
            reader->length--;
        }
        return CALCULATOR_USB_LINE_NONE;
    }
    if (character < 0x20 || character > 0x7e) {
        reader->invalid = true;
        return CALCULATOR_USB_LINE_NONE;
    }
    if (reader->overflow || reader->invalid) return CALCULATOR_USB_LINE_NONE;
    if (reader->length + 1u >= sizeof reader->line) {
        reader->overflow = true;
        return CALCULATOR_USB_LINE_NONE;
    }
    reader->line[reader->length++] = (char)character;
    return CALCULATOR_USB_LINE_NONE;
}
