#include "calculator_usb_protocol.h"

#include "calculator_engine.h"
#include "graph_model.h"
#include "statistics_engine.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CALCULATOR_FIRMWARE_VERSION "1.5.0"

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
            respond(response, response_size, "OK VAR\t%s\t%.17g",
                    calculator_variable_name(index),
                    state->symbols.variables[index]);
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
        state->symbols.variables[index] = value;
        effect->changed = true;
        effect->persistent_changed = true;
        respond(response, response_size, "OK VAR\t%s\t%.17g",
                calculator_variable_name(index), value);
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
    respond(response, response_size, "ERR UNKNOWN_ACTION %s", action);
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
                    "OK DIAG\tpage=%u\tangle=%s\thistory=%u\tstats=%u\tmode=%u"
                    "\tbasic=%u\tbasic_state=%s",
                    (unsigned int)context->state->page,
                    context->state->degrees ? "DEG" : "RAD",
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
