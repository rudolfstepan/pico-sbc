#include "basic_engine.h"

#include "tinyexpr.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const variable_names[BASIC_VARIABLE_COUNT] = {
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"
};

static const char *skip_space_const(const char *text) {
    while (*text && isspace((unsigned char)*text)) text++;
    return text;
}

static void trim_end(char *text) {
    size_t length = strlen(text);
    while (length && isspace((unsigned char)text[length - 1])) {
        text[--length] = '\0';
    }
}

static bool keyword(const char *text, const char *word, const char **after) {
    text = skip_space_const(text);
    size_t length = strlen(word);
    for (size_t i = 0; i < length; ++i) {
        if (!text[i]) return false;
        if (toupper((unsigned char)text[i]) !=
            toupper((unsigned char)word[i])) {
            return false;
        }
    }
    if (isalnum((unsigned char)text[length]) || text[length] == '_') {
        return false;
    }
    if (after) *after = skip_space_const(text + length);
    return true;
}

static const char *find_keyword(const char *text, const char *word) {
    size_t length = strlen(word);
    int depth = 0;
    for (const char *cursor = text; *cursor; ++cursor) {
        if (*cursor == '(') depth++;
        if (*cursor == ')' && depth > 0) depth--;
        if (depth) continue;
        bool left_boundary = cursor == text ||
            (!isalnum((unsigned char)cursor[-1]) && cursor[-1] != '_');
        if (!left_boundary) continue;
        size_t i = 0;
        while (i < length && cursor[i] &&
               toupper((unsigned char)cursor[i]) ==
               toupper((unsigned char)word[i])) {
            i++;
        }
        if (i == length && !isalnum((unsigned char)cursor[i]) &&
            cursor[i] != '_') {
            return cursor;
        }
    }
    return NULL;
}

static basic_status_t evaluate_expression(basic_engine_t *engine,
                                          const char *expression,
                                          double *result) {
    char normalized[BASIC_LINE_TEXT_CAPACITY];
    expression = skip_space_const(expression);
    size_t length = strlen(expression);
    if (!length || length >= sizeof normalized) return BASIC_STATUS_SYNTAX;
    for (size_t i = 0; i <= length; ++i) {
        normalized[i] = (char)tolower((unsigned char)expression[i]);
    }
    te_variable variables[BASIC_VARIABLE_COUNT];
    for (size_t i = 0; i < BASIC_VARIABLE_COUNT; ++i) {
        variables[i] = (te_variable){
            variable_names[i], &engine->variables[i], TE_VARIABLE, NULL
        };
    }
    int error = 0;
    te_expr *compiled = te_compile(normalized, variables,
                                   (int)BASIC_VARIABLE_COUNT, &error);
    if (!compiled) return BASIC_STATUS_SYNTAX;
    double value = te_eval(compiled);
    te_free(compiled);
    if (!isfinite(value)) return BASIC_STATUS_RANGE;
    if (result) *result = value;
    return BASIC_STATUS_OK;
}

static void push_output(basic_engine_t *engine, const char *text) {
    if (engine->output_count == BASIC_OUTPUT_LINES) {
        memmove(engine->output[0], engine->output[1],
                (BASIC_OUTPUT_LINES - 1u) * sizeof engine->output[0]);
        engine->output_count--;
    }
    snprintf(engine->output[engine->output_count], BASIC_OUTPUT_CAPACITY,
             "%s", text ? text : "");
    engine->output_count++;
    engine->output_revision++;
}

static size_t find_line(const basic_engine_t *engine, uint16_t number) {
    for (size_t i = 0; i < engine->program.count; ++i) {
        if (engine->program.lines[i].number == number) return i;
    }
    return BASIC_PROGRAM_MAX_LINES;
}

static basic_status_t parse_target(const char *text, uint16_t *target) {
    text = skip_space_const(text);
    if (!isdigit((unsigned char)*text)) return BASIC_STATUS_SYNTAX;
    errno = 0;
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 10);
    end = (char *)skip_space_const(end);
    if (errno == ERANGE || value == 0 || value > UINT16_MAX || *end) {
        return BASIC_STATUS_LINE_NUMBER;
    }
    *target = (uint16_t)value;
    return BASIC_STATUS_OK;
}

static void fail(basic_engine_t *engine, basic_status_t status,
                 uint16_t line_number) {
    char output[BASIC_OUTPUT_CAPACITY];
    engine->state = BASIC_RUN_ERROR;
    engine->status = status;
    engine->error_line = line_number;
    snprintf(output, sizeof output, "ERR %u: %s", (unsigned int)line_number,
             basic_status_text(status));
    push_output(engine, output);
}

static basic_status_t evaluate_condition(basic_engine_t *engine,
                                         const char *condition,
                                         bool *result) {
    const char *operator_at = NULL;
    const char *operator_text = NULL;
    static const char *const operators[] = {"<=", ">=", "<>", "=", "<", ">"};
    int depth = 0;
    for (const char *cursor = condition; *cursor && !operator_at; ++cursor) {
        if (*cursor == '(') depth++;
        if (*cursor == ')' && depth > 0) depth--;
        if (depth) continue;
        for (size_t i = 0; i < sizeof operators / sizeof operators[0]; ++i) {
            size_t length = strlen(operators[i]);
            if (strncmp(cursor, operators[i], length) == 0) {
                operator_at = cursor;
                operator_text = operators[i];
                break;
            }
        }
    }
    if (!operator_at) {
        double value = 0.0;
        basic_status_t status = evaluate_expression(engine, condition, &value);
        if (status == BASIC_STATUS_OK) *result = value != 0.0;
        return status;
    }

    char left[BASIC_LINE_TEXT_CAPACITY];
    char right[BASIC_LINE_TEXT_CAPACITY];
    size_t left_length = (size_t)(operator_at - condition);
    size_t operator_length = strlen(operator_text);
    if (!left_length || left_length >= sizeof left ||
        strlen(operator_at + operator_length) >= sizeof right) {
        return BASIC_STATUS_SYNTAX;
    }
    memcpy(left, condition, left_length);
    left[left_length] = '\0';
    snprintf(right, sizeof right, "%s", operator_at + operator_length);
    trim_end(left);
    trim_end(right);
    double a = 0.0;
    double b = 0.0;
    basic_status_t status = evaluate_expression(engine, left, &a);
    if (status != BASIC_STATUS_OK) return status;
    status = evaluate_expression(engine, right, &b);
    if (status != BASIC_STATUS_OK) return status;
    if (strcmp(operator_text, "<=") == 0) *result = a <= b;
    else if (strcmp(operator_text, ">=") == 0) *result = a >= b;
    else if (strcmp(operator_text, "<>") == 0) *result = a != b;
    else if (strcmp(operator_text, "=") == 0) *result = a == b;
    else if (strcmp(operator_text, "<") == 0) *result = a < b;
    else *result = a > b;
    return BASIC_STATUS_OK;
}

static size_t matching_next(const basic_engine_t *engine, size_t start) {
    unsigned int depth = 0;
    for (size_t i = start; i < engine->program.count; ++i) {
        const char *after = NULL;
        if (keyword(engine->program.lines[i].text, "FOR", &after)) {
            depth++;
        } else if (keyword(engine->program.lines[i].text, "NEXT", &after)) {
            if (!depth) return i;
            depth--;
        }
    }
    return BASIC_PROGRAM_MAX_LINES;
}

static basic_status_t execute_for(basic_engine_t *engine, const char *arguments,
                                  size_t next_pc, size_t *destination) {
    arguments = skip_space_const(arguments);
    if (!isalpha((unsigned char)arguments[0])) return BASIC_STATUS_SYNTAX;
    size_t variable = (size_t)(toupper((unsigned char)arguments[0]) - 'A');
    const char *cursor = skip_space_const(arguments + 1);
    if (*cursor != '=') return BASIC_STATUS_SYNTAX;
    cursor = skip_space_const(cursor + 1);
    const char *to = find_keyword(cursor, "TO");
    if (!to) return BASIC_STATUS_SYNTAX;
    char start_expression[BASIC_LINE_TEXT_CAPACITY];
    size_t start_length = (size_t)(to - cursor);
    if (!start_length || start_length >= sizeof start_expression) {
        return BASIC_STATUS_SYNTAX;
    }
    memcpy(start_expression, cursor, start_length);
    start_expression[start_length] = '\0';
    trim_end(start_expression);
    const char *limit_expression = skip_space_const(to + 2);
    const char *step_at = find_keyword(limit_expression, "STEP");
    char limit_text[BASIC_LINE_TEXT_CAPACITY];
    char step_text[BASIC_LINE_TEXT_CAPACITY] = "1";
    size_t limit_length = step_at
        ? (size_t)(step_at - limit_expression) : strlen(limit_expression);
    if (!limit_length || limit_length >= sizeof limit_text) {
        return BASIC_STATUS_SYNTAX;
    }
    memcpy(limit_text, limit_expression, limit_length);
    limit_text[limit_length] = '\0';
    trim_end(limit_text);
    if (step_at) {
        snprintf(step_text, sizeof step_text, "%s", skip_space_const(step_at + 4));
    }
    double start = 0.0;
    double limit = 0.0;
    double step = 0.0;
    basic_status_t status = evaluate_expression(engine, start_expression, &start);
    if (status != BASIC_STATUS_OK) return status;
    status = evaluate_expression(engine, limit_text, &limit);
    if (status != BASIC_STATUS_OK) return status;
    status = evaluate_expression(engine, step_text, &step);
    if (status != BASIC_STATUS_OK) return status;
    if (step == 0.0) return BASIC_STATUS_RANGE;
    engine->variables[variable] = start;
    bool active = step > 0.0 ? start <= limit : start >= limit;
    if (!active) {
        size_t next = matching_next(engine, next_pc);
        if (next == BASIC_PROGRAM_MAX_LINES) return BASIC_STATUS_NEXT_WITHOUT_FOR;
        *destination = next + 1u;
        return BASIC_STATUS_OK;
    }
    if (engine->loop_count == BASIC_LOOP_DEPTH) return BASIC_STATUS_LOOP_DEPTH;
    engine->loops[engine->loop_count++] = (basic_loop_t){
        .body_pc = next_pc,
        .variable = variable,
        .limit = limit,
        .step = step,
    };
    *destination = next_pc;
    return BASIC_STATUS_OK;
}

static basic_status_t execute_statement(basic_engine_t *engine,
                                        const basic_line_t *line,
                                        size_t next_pc, size_t *destination) {
    const char *statement = skip_space_const(line->text);
    const char *arguments = NULL;
    *destination = next_pc;
    if (!*statement || keyword(statement, "REM", &arguments)) {
        return BASIC_STATUS_OK;
    }
    if (keyword(statement, "END", &arguments)) {
        if (*arguments) return BASIC_STATUS_SYNTAX;
        engine->state = BASIC_RUN_FINISHED;
        engine->status = BASIC_STATUS_OK;
        engine->output_revision++;
        return BASIC_STATUS_OK;
    }
    if (keyword(statement, "CLS", &arguments)) {
        if (*arguments) return BASIC_STATUS_SYNTAX;
        engine->output_count = 0;
        engine->output_revision++;
        return BASIC_STATUS_OK;
    }
    if (keyword(statement, "PRINT", &arguments)) {
        if (!*arguments) {
            push_output(engine, "");
            return BASIC_STATUS_OK;
        }
        if (*arguments == '"') {
            const char *end = strchr(arguments + 1, '"');
            if (!end || *skip_space_const(end + 1)) return BASIC_STATUS_SYNTAX;
            char text[BASIC_OUTPUT_CAPACITY];
            size_t length = (size_t)(end - arguments - 1);
            if (length >= sizeof text) length = sizeof text - 1u;
            memcpy(text, arguments + 1, length);
            text[length] = '\0';
            push_output(engine, text);
            return BASIC_STATUS_OK;
        }
        double value = 0.0;
        basic_status_t status = evaluate_expression(engine, arguments, &value);
        if (status != BASIC_STATUS_OK) return status;
        char text[BASIC_OUTPUT_CAPACITY];
        snprintf(text, sizeof text, "%.12g", value);
        push_output(engine, text);
        return BASIC_STATUS_OK;
    }
    if (keyword(statement, "INPUT", &arguments)) {
        if (!isalpha((unsigned char)arguments[0]) ||
            *skip_space_const(arguments + 1)) {
            return BASIC_STATUS_SYNTAX;
        }
        engine->input_variable =
            (size_t)(toupper((unsigned char)arguments[0]) - 'A');
        engine->state = BASIC_RUN_INPUT;
        engine->status = BASIC_STATUS_INPUT_REQUIRED;
        engine->output_revision++;
        return BASIC_STATUS_OK;
    }
    if (keyword(statement, "GOTO", &arguments)) {
        uint16_t target = 0;
        basic_status_t status = parse_target(arguments, &target);
        if (status != BASIC_STATUS_OK) return status;
        size_t target_pc = find_line(engine, target);
        if (target_pc == BASIC_PROGRAM_MAX_LINES) return BASIC_STATUS_NO_TARGET;
        *destination = target_pc;
        return BASIC_STATUS_OK;
    }
    if (keyword(statement, "IF", &arguments)) {
        const char *then_at = find_keyword(arguments, "THEN");
        if (!then_at) return BASIC_STATUS_SYNTAX;
        char condition[BASIC_LINE_TEXT_CAPACITY];
        size_t length = (size_t)(then_at - arguments);
        if (!length || length >= sizeof condition) return BASIC_STATUS_SYNTAX;
        memcpy(condition, arguments, length);
        condition[length] = '\0';
        trim_end(condition);
        bool condition_result = false;
        basic_status_t status = evaluate_condition(engine, condition,
                                                   &condition_result);
        if (status != BASIC_STATUS_OK || !condition_result) return status;
        uint16_t target = 0;
        status = parse_target(skip_space_const(then_at + 4), &target);
        if (status != BASIC_STATUS_OK) return status;
        size_t target_pc = find_line(engine, target);
        if (target_pc == BASIC_PROGRAM_MAX_LINES) return BASIC_STATUS_NO_TARGET;
        *destination = target_pc;
        return BASIC_STATUS_OK;
    }
    if (keyword(statement, "FOR", &arguments)) {
        return execute_for(engine, arguments, next_pc, destination);
    }
    if (keyword(statement, "NEXT", &arguments)) {
        if (!engine->loop_count) return BASIC_STATUS_NEXT_WITHOUT_FOR;
        basic_loop_t *loop = &engine->loops[engine->loop_count - 1u];
        if (*arguments) {
            if (!isalpha((unsigned char)arguments[0]) ||
                *skip_space_const(arguments + 1) ||
                (size_t)(toupper((unsigned char)arguments[0]) - 'A') !=
                    loop->variable) {
                return BASIC_STATUS_NEXT_WITHOUT_FOR;
            }
        }
        engine->variables[loop->variable] += loop->step;
        if (!isfinite(engine->variables[loop->variable])) {
            return BASIC_STATUS_RANGE;
        }
        bool active = loop->step > 0.0
            ? engine->variables[loop->variable] <= loop->limit
            : engine->variables[loop->variable] >= loop->limit;
        if (active) {
            *destination = loop->body_pc;
        } else {
            engine->loop_count--;
        }
        return BASIC_STATUS_OK;
    }

    if (keyword(statement, "LET", &arguments)) statement = arguments;
    statement = skip_space_const(statement);
    if (!isalpha((unsigned char)statement[0])) return BASIC_STATUS_SYNTAX;
    size_t variable = (size_t)(toupper((unsigned char)statement[0]) - 'A');
    const char *equals = skip_space_const(statement + 1);
    if (*equals != '=') return BASIC_STATUS_SYNTAX;
    double value = 0.0;
    basic_status_t status = evaluate_expression(engine,
                                                skip_space_const(equals + 1),
                                                &value);
    if (status == BASIC_STATUS_OK) engine->variables[variable] = value;
    return status;
}

void basic_engine_init(basic_engine_t *engine) {
    if (!engine) return;
    memset(engine, 0, sizeof *engine);
    engine->state = BASIC_RUN_STOPPED;
    engine->status = BASIC_STATUS_OK;
}

bool basic_program_is_valid(const basic_program_t *program) {
    if (!program || program->count > BASIC_PROGRAM_MAX_LINES) return false;
    uint16_t previous = 0;
    for (size_t i = 0; i < program->count; ++i) {
        if (!program->lines[i].number || program->lines[i].number <= previous ||
            !program->lines[i].text[0] ||
            !memchr(program->lines[i].text, '\0', BASIC_LINE_TEXT_CAPACITY)) {
            return false;
        }
        previous = program->lines[i].number;
    }
    return true;
}

basic_status_t basic_engine_set_program(basic_engine_t *engine,
                                        const basic_program_t *program) {
    if (!engine || !basic_program_is_valid(program)) return BASIC_STATUS_SYNTAX;
    engine->program = *program;
    basic_engine_stop(engine);
    return BASIC_STATUS_OK;
}

basic_status_t basic_engine_store_line(basic_engine_t *engine,
                                       const char *source) {
    if (!engine || !source) return BASIC_STATUS_SYNTAX;
    if (engine->state == BASIC_RUN_RUNNING || engine->state == BASIC_RUN_INPUT) {
        return BASIC_STATUS_RUNNING;
    }
    source = skip_space_const(source);
    if (!isdigit((unsigned char)*source)) return BASIC_STATUS_LINE_NUMBER;
    errno = 0;
    char *end = NULL;
    unsigned long number = strtoul(source, &end, 10);
    if (errno == ERANGE || number == 0 || number > UINT16_MAX) {
        return BASIC_STATUS_LINE_NUMBER;
    }
    const char *text = skip_space_const(end);
    char normalized[BASIC_LINE_TEXT_CAPACITY];
    if (strlen(text) >= sizeof normalized) return BASIC_STATUS_LINE_TOO_LONG;
    snprintf(normalized, sizeof normalized, "%s", text);
    trim_end(normalized);
    size_t existing = find_line(engine, (uint16_t)number);
    if (!normalized[0]) {
        if (existing < engine->program.count) {
            memmove(&engine->program.lines[existing],
                    &engine->program.lines[existing + 1u],
                    (engine->program.count - existing - 1u) *
                    sizeof engine->program.lines[0]);
            engine->program.count--;
        }
        return BASIC_STATUS_OK;
    }
    if (existing < engine->program.count) {
        snprintf(engine->program.lines[existing].text,
                 BASIC_LINE_TEXT_CAPACITY, "%s", normalized);
        return BASIC_STATUS_OK;
    }
    if (engine->program.count == BASIC_PROGRAM_MAX_LINES) {
        return BASIC_STATUS_FULL;
    }
    size_t insert = 0;
    while (insert < engine->program.count &&
           engine->program.lines[insert].number < number) {
        insert++;
    }
    memmove(&engine->program.lines[insert + 1u],
            &engine->program.lines[insert],
            (engine->program.count - insert) * sizeof engine->program.lines[0]);
    engine->program.lines[insert].number = (uint16_t)number;
    snprintf(engine->program.lines[insert].text, BASIC_LINE_TEXT_CAPACITY,
             "%s", normalized);
    engine->program.count++;
    return BASIC_STATUS_OK;
}

void basic_engine_clear_program(basic_engine_t *engine) {
    if (!engine) return;
    basic_engine_stop(engine);
    memset(&engine->program, 0, sizeof engine->program);
    engine->output_count = 0;
    engine->output_revision++;
}

basic_status_t basic_engine_run(basic_engine_t *engine) {
    if (!engine || !engine->program.count) return BASIC_STATUS_EMPTY;
    memset(engine->variables, 0, sizeof engine->variables);
    engine->output_count = 0;
    engine->pc = 0;
    engine->instruction_count = 0;
    engine->loop_count = 0;
    engine->error_line = 0;
    engine->state = BASIC_RUN_RUNNING;
    engine->status = BASIC_STATUS_OK;
    engine->output_revision++;
    return BASIC_STATUS_OK;
}

void basic_engine_stop(basic_engine_t *engine) {
    if (!engine) return;
    engine->state = BASIC_RUN_STOPPED;
    engine->status = BASIC_STATUS_OK;
    engine->loop_count = 0;
    engine->output_revision++;
}

basic_status_t basic_engine_step(basic_engine_t *engine) {
    if (!engine) return BASIC_STATUS_SYNTAX;
    if (engine->state == BASIC_RUN_INPUT) return BASIC_STATUS_INPUT_REQUIRED;
    if (engine->state != BASIC_RUN_RUNNING) return engine->status;
    if (engine->instruction_count >= BASIC_INSTRUCTION_LIMIT) {
        uint16_t line = engine->pc < engine->program.count
            ? engine->program.lines[engine->pc].number : 0;
        fail(engine, BASIC_STATUS_LIMIT, line);
        return BASIC_STATUS_LIMIT;
    }
    if (engine->pc >= engine->program.count) {
        engine->state = BASIC_RUN_FINISHED;
        engine->status = BASIC_STATUS_OK;
        engine->output_revision++;
        return BASIC_STATUS_OK;
    }
    const basic_line_t *line = &engine->program.lines[engine->pc];
    size_t destination = engine->pc + 1u;
    engine->instruction_count++;
    basic_status_t status = execute_statement(engine, line, destination,
                                              &destination);
    if (status != BASIC_STATUS_OK) {
        fail(engine, status, line->number);
        return status;
    }
    engine->pc = destination;
    if (engine->state == BASIC_RUN_RUNNING &&
        engine->pc >= engine->program.count) {
        engine->state = BASIC_RUN_FINISHED;
        engine->output_revision++;
    }
    return BASIC_STATUS_OK;
}

basic_status_t basic_engine_submit_input(basic_engine_t *engine,
                                         const char *input) {
    if (!engine || engine->state != BASIC_RUN_INPUT) {
        return BASIC_STATUS_INPUT_REQUIRED;
    }
    double value = 0.0;
    basic_status_t status = evaluate_expression(engine, input, &value);
    if (status != BASIC_STATUS_OK) return status;
    engine->variables[engine->input_variable] = value;
    char echo[BASIC_OUTPUT_CAPACITY];
    snprintf(echo, sizeof echo, "? %s", input);
    push_output(engine, echo);
    engine->state = BASIC_RUN_RUNNING;
    engine->status = BASIC_STATUS_OK;
    engine->output_revision++;
    return BASIC_STATUS_OK;
}

const char *basic_status_text(basic_status_t status) {
    switch (status) {
        case BASIC_STATUS_OK: return "OK";
        case BASIC_STATUS_EMPTY: return "NO PROGRAM";
        case BASIC_STATUS_FULL: return "PROGRAM FULL";
        case BASIC_STATUS_LINE_NUMBER: return "LINE NUMBER";
        case BASIC_STATUS_LINE_TOO_LONG: return "LINE TOO LONG";
        case BASIC_STATUS_SYNTAX: return "SYNTAX";
        case BASIC_STATUS_RANGE: return "NUMBER RANGE";
        case BASIC_STATUS_NO_TARGET: return "NO TARGET";
        case BASIC_STATUS_NEXT_WITHOUT_FOR: return "NEXT WITHOUT FOR";
        case BASIC_STATUS_LOOP_DEPTH: return "LOOP DEPTH";
        case BASIC_STATUS_RUNNING: return "PROGRAM RUNNING";
        case BASIC_STATUS_INPUT_REQUIRED: return "INPUT";
        case BASIC_STATUS_LIMIT: return "STEP LIMIT";
        default: return "ERROR";
    }
}
