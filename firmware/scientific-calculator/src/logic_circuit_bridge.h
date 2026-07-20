#ifndef LOGIC_CIRCUIT_BRIDGE_H
#define LOGIC_CIRCUIT_BRIDGE_H

#include "circuit_model.h"
#include "logic_engine.h"

#include <stddef.h>
#include <stdint.h>

typedef enum {
    LOGIC_CIRCUIT_OK,
    LOGIC_CIRCUIT_EMPTY,
    LOGIC_CIRCUIT_TOO_COMPLEX,
    LOGIC_CIRCUIT_NO_OUTPUT,
    LOGIC_CIRCUIT_INVALID_INPUT,
    LOGIC_CIRCUIT_DUPLICATE_INPUT,
    LOGIC_CIRCUIT_CYCLE,
    LOGIC_CIRCUIT_OUTPUT_FULL,
    LOGIC_CIRCUIT_INVALID
} logic_circuit_status_t;

logic_circuit_status_t logic_circuit_from_program(
    const logic_program_t *program, uint8_t assignment,
    circuit_model_t *model);
logic_circuit_status_t logic_circuit_to_expression(
    const circuit_model_t *model, uint8_t target,
    char *expression, size_t expression_size,
    uint8_t *assignment);
const char *logic_circuit_status_text(logic_circuit_status_t status);

#endif
