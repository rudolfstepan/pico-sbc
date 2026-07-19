#ifndef BASIC_ENGINE_H
#define BASIC_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BASIC_PROGRAM_MAX_LINES 20u
#define BASIC_LINE_TEXT_CAPACITY 64u
#define BASIC_VARIABLE_COUNT 26u
#define BASIC_OUTPUT_LINES 16u
#define BASIC_OUTPUT_CAPACITY 79u
#define BASIC_LOOP_DEPTH 4u
#define BASIC_INSTRUCTION_LIMIT 5000u

typedef struct {
    uint16_t number;
    char text[BASIC_LINE_TEXT_CAPACITY];
} basic_line_t;

typedef struct {
    basic_line_t lines[BASIC_PROGRAM_MAX_LINES];
    size_t count;
} basic_program_t;

typedef enum {
    BASIC_STATUS_OK,
    BASIC_STATUS_EMPTY,
    BASIC_STATUS_FULL,
    BASIC_STATUS_LINE_NUMBER,
    BASIC_STATUS_LINE_TOO_LONG,
    BASIC_STATUS_SYNTAX,
    BASIC_STATUS_RANGE,
    BASIC_STATUS_NO_TARGET,
    BASIC_STATUS_NEXT_WITHOUT_FOR,
    BASIC_STATUS_LOOP_DEPTH,
    BASIC_STATUS_RUNNING,
    BASIC_STATUS_INPUT_REQUIRED,
    BASIC_STATUS_LIMIT
} basic_status_t;

typedef enum {
    BASIC_RUN_STOPPED,
    BASIC_RUN_RUNNING,
    BASIC_RUN_INPUT,
    BASIC_RUN_FINISHED,
    BASIC_RUN_ERROR
} basic_run_state_t;

typedef struct {
    size_t body_pc;
    size_t variable;
    double limit;
    double step;
} basic_loop_t;

typedef struct {
    basic_program_t program;
    double variables[BASIC_VARIABLE_COUNT];
    char output[BASIC_OUTPUT_LINES][BASIC_OUTPUT_CAPACITY];
    size_t output_count;
    size_t pc;
    size_t instruction_count;
    basic_loop_t loops[BASIC_LOOP_DEPTH];
    size_t loop_count;
    size_t input_variable;
    uint16_t error_line;
    unsigned int output_revision;
    basic_run_state_t state;
    basic_status_t status;
} basic_engine_t;

void basic_engine_init(basic_engine_t *engine);
bool basic_program_is_valid(const basic_program_t *program);
basic_status_t basic_engine_set_program(basic_engine_t *engine,
                                        const basic_program_t *program);
basic_status_t basic_engine_store_line(basic_engine_t *engine,
                                       const char *source);
void basic_engine_clear_program(basic_engine_t *engine);
basic_status_t basic_engine_run(basic_engine_t *engine);
void basic_engine_stop(basic_engine_t *engine);
basic_status_t basic_engine_step(basic_engine_t *engine);
basic_status_t basic_engine_submit_input(basic_engine_t *engine,
                                         const char *input);
const char *basic_status_text(basic_status_t status);

#endif
