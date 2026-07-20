#include "circuit_model.h"

#include <stdio.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static size_t node_count(const circuit_model_t *model) {
    size_t count = 0;
    for (size_t i = 0; i < CIRCUIT_NODE_CAPACITY; ++i) {
        if (model->nodes[i].used) ++count;
    }
    return count;
}

static size_t wire_count(const circuit_model_t *model) {
    size_t count = 0;
    for (size_t i = 0; i < CIRCUIT_WIRE_CAPACITY; ++i) {
        if (model->wires[i].used) ++count;
    }
    return count;
}

static int check_binary_gate(circuit_gate_type_t type,
                             const bool expected[4]) {
    circuit_model_t model;
    circuit_model_clear(&model);
    int a = circuit_model_add(&model, CIRCUIT_GATE_INPUT, 0, 80);
    int b = circuit_model_add(&model, CIRCUIT_GATE_INPUT, 0, 180);
    int gate = circuit_model_add(&model, type, 160, 120);
    int output = circuit_model_add(&model, CIRCUIT_GATE_OUTPUT, 320, 120);
    CHECK(a >= 0 && b >= 0 && gate >= 0 && output >= 0);
    CHECK(circuit_model_connect(&model, (uint8_t)a, (uint8_t)gate, 0u));
    CHECK(circuit_model_connect(&model, (uint8_t)b, (uint8_t)gate, 1u));
    CHECK(circuit_model_connect(&model, (uint8_t)gate,
                                (uint8_t)output, 0u));
    for (unsigned int assignment = 0; assignment < 4u; ++assignment) {
        model.nodes[a].input_value = (assignment & 2u) != 0u;
        model.nodes[b].input_value = (assignment & 1u) != 0u;
        CHECK(circuit_model_evaluate(&model));
        CHECK(model.nodes[output].output_value == expected[assignment]);
    }
    return 0;
}

int main(void) {
    static const bool and_values[4] = {false, false, false, true};
    static const bool or_values[4] = {false, true, true, true};
    static const bool xor_values[4] = {false, true, true, false};
    static const bool nand_values[4] = {true, true, true, false};
    static const bool nor_values[4] = {true, false, false, false};
    static const bool xnor_values[4] = {true, false, false, true};
    CHECK(check_binary_gate(CIRCUIT_GATE_AND, and_values) == 0);
    CHECK(check_binary_gate(CIRCUIT_GATE_OR, or_values) == 0);
    CHECK(check_binary_gate(CIRCUIT_GATE_XOR, xor_values) == 0);
    CHECK(check_binary_gate(CIRCUIT_GATE_NAND, nand_values) == 0);
    CHECK(check_binary_gate(CIRCUIT_GATE_NOR, nor_values) == 0);
    CHECK(check_binary_gate(CIRCUIT_GATE_XNOR, xnor_values) == 0);

    circuit_model_t model;
    circuit_model_init(&model);
    CHECK(node_count(&model) == 4u);
    CHECK(wire_count(&model) == 3u);
    CHECK(model.nodes[0].type == CIRCUIT_GATE_INPUT);
    CHECK(model.nodes[2].type == CIRCUIT_GATE_AND);
    CHECK(model.nodes[3].type == CIRCUIT_GATE_OUTPUT);
    CHECK(!model.nodes[3].output_value);

    CHECK(circuit_model_toggle_input(&model, 0u));
    CHECK(!model.nodes[3].output_value);
    CHECK(circuit_model_toggle_input(&model, 1u));
    CHECK(model.nodes[2].output_value);
    CHECK(model.nodes[3].output_value);

    int inverter = circuit_model_add(&model, CIRCUIT_GATE_NOT, 500, 100);
    CHECK(inverter >= 0);
    CHECK(circuit_model_connect(&model, 2u, (uint8_t)inverter, 0u));
    CHECK(!circuit_model_connect(&model, (uint8_t)inverter, 2u, 0u));
    CHECK(!model.cycle_detected);

    CHECK(circuit_model_connect(&model, 1u, 2u, 0u));
    int input_wire = circuit_model_wire_for_input(&model, 2u, 0u);
    CHECK(input_wire >= 0);
    CHECK(model.wires[input_wire].source == 1u);
    CHECK(wire_count(&model) == 4u);

    CHECK(circuit_model_set_type(&model, 2u, CIRCUIT_GATE_NOT));
    CHECK(circuit_model_wire_for_input(&model, 2u, 1u) < 0);
    CHECK(wire_count(&model) == 3u);
    CHECK(circuit_model_set_type(&model, 2u, CIRCUIT_GATE_OUTPUT));
    CHECK(circuit_model_wire_for_input(&model, (uint8_t)inverter, 0u) < 0);
    CHECK(circuit_model_remove(&model, 1u));
    CHECK(circuit_model_wire_for_input(&model, 2u, 0u) < 0);

    circuit_model_clear(&model);
    for (size_t i = 0; i < CIRCUIT_NODE_CAPACITY; ++i) {
        CHECK(circuit_model_add(&model, CIRCUIT_GATE_INPUT,
                                (int)i * 10, 80) >= 0);
    }
    CHECK(circuit_model_add(&model, CIRCUIT_GATE_AND, 0, 0) < 0);

    puts("circuit model tests passed");
    return 0;
}
