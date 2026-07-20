#ifndef LOGIC_ENGINE_H
#define LOGIC_ENGINE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LOGIC_VARIABLE_COUNT 6u
#define LOGIC_NODE_CAPACITY 96u
#define LOGIC_FORM_CAPACITY 2048u

typedef enum {
    LOGIC_STATUS_OK,
    LOGIC_STATUS_EMPTY,
    LOGIC_STATUS_SYNTAX,
    LOGIC_STATUS_TOO_COMPLEX,
    LOGIC_STATUS_OUTPUT_FULL
} logic_status_t;

typedef enum {
    LOGIC_NODE_CONSTANT,
    LOGIC_NODE_VARIABLE,
    LOGIC_NODE_NOT,
    LOGIC_NODE_AND,
    LOGIC_NODE_OR,
    LOGIC_NODE_XOR,
    LOGIC_NODE_NAND,
    LOGIC_NODE_NOR,
    LOGIC_NODE_IMPLIES,
    LOGIC_NODE_XNOR
} logic_node_kind_t;

typedef struct {
    logic_node_kind_t kind;
    uint8_t left;
    uint8_t right;
    uint8_t value;
} logic_node_t;

typedef struct {
    logic_node_t nodes[LOGIC_NODE_CAPACITY];
    size_t node_count;
    uint8_t root;
    uint8_t variable_mask;
} logic_program_t;

logic_status_t logic_engine_compile(const char *expression,
                                    logic_program_t *program,
                                    int *error_position);
bool logic_engine_evaluate(const logic_program_t *program,
                           uint8_t assignment);
size_t logic_engine_variable_count(const logic_program_t *program);
size_t logic_engine_truth_row_count(const logic_program_t *program);
uint8_t logic_engine_assignment_for_row(const logic_program_t *program,
                                        size_t row);
logic_status_t logic_engine_format_canonical(
    const logic_program_t *program, bool dnf,
    char *output, size_t output_size);
logic_status_t logic_engine_format_simplified(
    const logic_program_t *program, bool dnf,
    char *output, size_t output_size);
const char *logic_engine_status_text(logic_status_t status);

#endif
