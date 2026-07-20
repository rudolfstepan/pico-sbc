#include "logic_circuit_bridge.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static bool equivalent(const logic_program_t *first,
                       const logic_program_t *second) {
    for (uint8_t assignment = 0u; assignment < 64u; ++assignment) {
        if (logic_engine_evaluate(first, assignment) !=
            logic_engine_evaluate(second, assignment)) {
            return false;
        }
    }
    return true;
}

static size_t count_type(const circuit_model_t *model,
                         circuit_gate_type_t type) {
    size_t count = 0u;
    for (size_t i = 0; i < CIRCUIT_NODE_CAPACITY; ++i) {
        if (model->nodes[i].used && model->nodes[i].type == type) count++;
    }
    return count;
}

static int round_trip(const char *expression, uint8_t assignment) {
    logic_program_t source;
    logic_program_t restored;
    circuit_model_t circuit;
    char generated[192];
    int error = 0;
    uint8_t restored_assignment = 0u;
    CHECK(logic_engine_compile(expression, &source, &error) ==
          LOGIC_STATUS_OK);
    CHECK(logic_circuit_from_program(&source, assignment, &circuit) ==
          LOGIC_CIRCUIT_OK);
    CHECK(logic_circuit_to_expression(
              &circuit, CIRCUIT_NODE_NONE, generated, sizeof generated,
              &restored_assignment) == LOGIC_CIRCUIT_OK);
    CHECK(logic_engine_compile(generated, &restored, &error) ==
          LOGIC_STATUS_OK);
    CHECK(equivalent(&source, &restored));
    CHECK((restored_assignment & source.variable_mask) ==
          (assignment & source.variable_mask));
    for (size_t i = 0; i < CIRCUIT_NODE_CAPACITY; ++i) {
        if (!circuit.nodes[i].used) continue;
        CHECK(circuit.nodes[i].x >= 0 &&
              circuit.nodes[i].x <= CIRCUIT_WORLD_WIDTH);
        CHECK(circuit.nodes[i].y >= 0 &&
              circuit.nodes[i].y <= CIRCUIT_WORLD_HEIGHT);
    }
    return 0;
}

int main(void) {
    CHECK(round_trip("A&(!B|C)", 0x05u) == 0);
    CHECK(round_trip("A NAND B", 0x03u) == 0);
    CHECK(round_trip("A NOR B", 0x01u) == 0);
    CHECK(round_trip("A IMPLIES B", 0x01u) == 0);
    CHECK(round_trip("A XNOR (B^C)", 0x06u) == 0);
    CHECK(round_trip("!(A|B)&(C^D)", 0x09u) == 0);
    CHECK(round_trip("1", 0u) == 0);
    CHECK(round_trip("0", 0u) == 0);

    logic_program_t repeated;
    circuit_model_t circuit;
    int error = 0;
    CHECK(logic_engine_compile("(A&B)|(A&C)", &repeated, &error) ==
          LOGIC_STATUS_OK);
    CHECK(logic_circuit_from_program(&repeated, 0x05u, &circuit) ==
          LOGIC_CIRCUIT_OK);
    CHECK(count_type(&circuit, CIRCUIT_GATE_INPUT) == 3u);
    CHECK(count_type(&circuit, CIRCUIT_GATE_OUTPUT) == 1u);

    circuit_model_t selected;
    circuit_model_clear(&selected);
    int a = circuit_model_add(&selected, CIRCUIT_GATE_INPUT, 20, 80);
    int b = circuit_model_add(&selected, CIRCUIT_GATE_INPUT, 20, 180);
    int and_gate = circuit_model_add(&selected, CIRCUIT_GATE_AND, 180, 80);
    int or_gate = circuit_model_add(&selected, CIRCUIT_GATE_OR, 180, 180);
    int y = circuit_model_add(&selected, CIRCUIT_GATE_OUTPUT, 340, 80);
    int z = circuit_model_add(&selected, CIRCUIT_GATE_OUTPUT, 340, 180);
    CHECK(a >= 0 && b >= 0 && and_gate >= 0 && or_gate >= 0 &&
          y >= 0 && z >= 0);
    CHECK(circuit_model_connect(&selected, (uint8_t)a,
                                (uint8_t)and_gate, 0u));
    CHECK(circuit_model_connect(&selected, (uint8_t)b,
                                (uint8_t)and_gate, 1u));
    CHECK(circuit_model_connect(&selected, (uint8_t)a,
                                (uint8_t)or_gate, 0u));
    CHECK(circuit_model_connect(&selected, (uint8_t)b,
                                (uint8_t)or_gate, 1u));
    CHECK(circuit_model_connect(&selected, (uint8_t)and_gate,
                                (uint8_t)y, 0u));
    CHECK(circuit_model_connect(&selected, (uint8_t)or_gate,
                                (uint8_t)z, 0u));
    char expression[192];
    CHECK(logic_circuit_to_expression(
              &selected, (uint8_t)z, expression, sizeof expression,
              NULL) == LOGIC_CIRCUIT_OK);
    CHECK(strcmp(expression, "(A|B)") == 0);
    CHECK(logic_circuit_to_expression(
              &selected, CIRCUIT_NODE_NONE, expression, sizeof expression,
              NULL) == LOGIC_CIRCUIT_OK);
    CHECK(strcmp(expression, "(A&B)") == 0);

    snprintf(selected.nodes[a].label, sizeof selected.nodes[a].label, "TEMP");
    CHECK(logic_circuit_to_expression(
              &selected, (uint8_t)y, expression, sizeof expression,
              NULL) == LOGIC_CIRCUIT_INVALID_INPUT);
    snprintf(selected.nodes[a].label, sizeof selected.nodes[a].label, "B");
    CHECK(logic_circuit_to_expression(
              &selected, (uint8_t)y, expression, sizeof expression,
              NULL) == LOGIC_CIRCUIT_DUPLICATE_INPUT);

    circuit_model_clear(&selected);
    CHECK(logic_circuit_to_expression(
              &selected, CIRCUIT_NODE_NONE, expression, sizeof expression,
              NULL) == LOGIC_CIRCUIT_NO_OUTPUT);

    puts("logic circuit bridge tests passed");
    return 0;
}
