#ifndef CIRCUIT_MODEL_H
#define CIRCUIT_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CIRCUIT_NODE_CAPACITY 24u
#define CIRCUIT_WIRE_CAPACITY 48u
#define CIRCUIT_NODE_NONE UINT8_MAX
#define CIRCUIT_WORLD_WIDTH 1600
#define CIRCUIT_WORLD_HEIGHT 1200

typedef enum {
    CIRCUIT_GATE_INPUT,
    CIRCUIT_GATE_OUTPUT,
    CIRCUIT_GATE_NOT,
    CIRCUIT_GATE_AND,
    CIRCUIT_GATE_OR,
    CIRCUIT_GATE_XOR,
    CIRCUIT_GATE_NAND,
    CIRCUIT_GATE_NOR,
    CIRCUIT_GATE_IMPLIES,
    CIRCUIT_GATE_XNOR,
    CIRCUIT_GATE_COUNT
} circuit_gate_type_t;

typedef struct {
    bool used;
    circuit_gate_type_t type;
    int16_t x;
    int16_t y;
    bool input_value;
    bool output_value;
    char label[8];
} circuit_node_t;

typedef struct {
    bool used;
    uint8_t source;
    uint8_t destination;
    uint8_t destination_input;
} circuit_wire_t;

typedef struct {
    circuit_node_t nodes[CIRCUIT_NODE_CAPACITY];
    circuit_wire_t wires[CIRCUIT_WIRE_CAPACITY];
    uint8_t next_input_label;
    uint8_t next_output_label;
    uint8_t next_gate_label;
    bool cycle_detected;
} circuit_model_t;

void circuit_model_clear(circuit_model_t *model);
void circuit_model_init(circuit_model_t *model);
int circuit_model_add(circuit_model_t *model, circuit_gate_type_t type,
                      int x, int y);
bool circuit_model_remove(circuit_model_t *model, uint8_t node);
bool circuit_model_move(circuit_model_t *model, uint8_t node, int x, int y);
bool circuit_model_set_type(circuit_model_t *model, uint8_t node,
                            circuit_gate_type_t type);
bool circuit_model_toggle_input(circuit_model_t *model, uint8_t node);
bool circuit_model_connect(circuit_model_t *model, uint8_t source,
                           uint8_t destination, uint8_t destination_input);
bool circuit_model_disconnect_input(circuit_model_t *model,
                                    uint8_t destination,
                                    uint8_t destination_input);
int circuit_model_wire_for_input(const circuit_model_t *model,
                                 uint8_t destination,
                                 uint8_t destination_input);
bool circuit_model_evaluate(circuit_model_t *model);

size_t circuit_gate_input_count(circuit_gate_type_t type);
bool circuit_gate_has_output(circuit_gate_type_t type);
const char *circuit_gate_name(circuit_gate_type_t type);

#endif
