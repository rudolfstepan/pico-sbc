#include "circuit_model.h"

#include <stdio.h>
#include <string.h>

static bool valid_node(const circuit_model_t *model, uint8_t node) {
    return model && node < CIRCUIT_NODE_CAPACITY && model->nodes[node].used;
}

size_t circuit_gate_input_count(circuit_gate_type_t type) {
    switch (type) {
        case CIRCUIT_GATE_INPUT:
            return 0u;
        case CIRCUIT_GATE_OUTPUT:
        case CIRCUIT_GATE_NOT:
            return 1u;
        case CIRCUIT_GATE_AND:
        case CIRCUIT_GATE_OR:
        case CIRCUIT_GATE_XOR:
        case CIRCUIT_GATE_NAND:
        case CIRCUIT_GATE_NOR:
        case CIRCUIT_GATE_IMPLIES:
        case CIRCUIT_GATE_XNOR:
            return 2u;
        case CIRCUIT_GATE_COUNT:
        default:
            return 0u;
    }
}

bool circuit_gate_has_output(circuit_gate_type_t type) {
    return type != CIRCUIT_GATE_OUTPUT && type < CIRCUIT_GATE_COUNT;
}

const char *circuit_gate_name(circuit_gate_type_t type) {
    static const char *const names[] = {
        "INPUT", "OUTPUT", "NOT", "AND", "OR",
        "XOR", "NAND", "NOR", "IMPLIES", "XNOR"
    };
    return type < CIRCUIT_GATE_COUNT ? names[type] : "?";
}

void circuit_model_clear(circuit_model_t *model) {
    if (!model) return;
    memset(model, 0, sizeof *model);
}

static void assign_label(circuit_model_t *model, circuit_node_t *node) {
    if (node->type == CIRCUIT_GATE_INPUT) {
        unsigned int index = model->next_input_label++;
        if (index < 26u) {
            snprintf(node->label, sizeof node->label, "%c", (int)('A' + index));
        } else {
            snprintf(node->label, sizeof node->label, "I%u", index + 1u);
        }
    } else if (node->type == CIRCUIT_GATE_OUTPUT) {
        unsigned int index = model->next_output_label++;
        if (index < 2u) {
            snprintf(node->label, sizeof node->label, "%c", (int)('Y' + index));
        } else {
            snprintf(node->label, sizeof node->label, "O%u", index + 1u);
        }
    } else {
        snprintf(node->label, sizeof node->label, "G%u",
                 (unsigned int)model->next_gate_label++ + 1u);
    }
}

int circuit_model_add(circuit_model_t *model, circuit_gate_type_t type,
                      int x, int y) {
    if (!model || type >= CIRCUIT_GATE_COUNT) return -1;
    for (size_t i = 0; i < CIRCUIT_NODE_CAPACITY; ++i) {
        if (model->nodes[i].used) continue;
        circuit_node_t *node = &model->nodes[i];
        memset(node, 0, sizeof *node);
        node->used = true;
        node->type = type;
        node->x = (int16_t)(x < 0 ? 0 :
            (x > CIRCUIT_WORLD_WIDTH ? CIRCUIT_WORLD_WIDTH : x));
        node->y = (int16_t)(y < 0 ? 0 :
            (y > CIRCUIT_WORLD_HEIGHT ? CIRCUIT_WORLD_HEIGHT : y));
        assign_label(model, node);
        return (int)i;
    }
    return -1;
}

bool circuit_model_remove(circuit_model_t *model, uint8_t node) {
    if (!valid_node(model, node)) return false;
    model->nodes[node].used = false;
    for (size_t i = 0; i < CIRCUIT_WIRE_CAPACITY; ++i) {
        if (model->wires[i].used &&
            (model->wires[i].source == node ||
             model->wires[i].destination == node)) {
            model->wires[i].used = false;
        }
    }
    circuit_model_evaluate(model);
    return true;
}

bool circuit_model_move(circuit_model_t *model, uint8_t node, int x, int y) {
    if (!valid_node(model, node)) return false;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > CIRCUIT_WORLD_WIDTH) x = CIRCUIT_WORLD_WIDTH;
    if (y > CIRCUIT_WORLD_HEIGHT) y = CIRCUIT_WORLD_HEIGHT;
    model->nodes[node].x = (int16_t)x;
    model->nodes[node].y = (int16_t)y;
    return true;
}

int circuit_model_wire_for_input(const circuit_model_t *model,
                                 uint8_t destination,
                                 uint8_t destination_input) {
    if (!model) return -1;
    for (size_t i = 0; i < CIRCUIT_WIRE_CAPACITY; ++i) {
        const circuit_wire_t *wire = &model->wires[i];
        if (wire->used && wire->destination == destination &&
            wire->destination_input == destination_input) {
            return (int)i;
        }
    }
    return -1;
}

bool circuit_model_disconnect_input(circuit_model_t *model,
                                    uint8_t destination,
                                    uint8_t destination_input) {
    int wire = circuit_model_wire_for_input(model, destination,
                                            destination_input);
    if (wire < 0) return false;
    model->wires[wire].used = false;
    circuit_model_evaluate(model);
    return true;
}

static bool path_reaches(const circuit_model_t *model, uint8_t from,
                         uint8_t target, bool *visited) {
    if (from == target) return true;
    if (visited[from]) return false;
    visited[from] = true;
    for (size_t i = 0; i < CIRCUIT_WIRE_CAPACITY; ++i) {
        const circuit_wire_t *wire = &model->wires[i];
        if (!wire->used || wire->source != from) continue;
        if (path_reaches(model, wire->destination, target, visited)) {
            return true;
        }
    }
    return false;
}

bool circuit_model_connect(circuit_model_t *model, uint8_t source,
                           uint8_t destination, uint8_t destination_input) {
    if (!valid_node(model, source) || !valid_node(model, destination) ||
        source == destination ||
        !circuit_gate_has_output(model->nodes[source].type) ||
        destination_input >=
            circuit_gate_input_count(model->nodes[destination].type)) {
        return false;
    }

    bool visited[CIRCUIT_NODE_CAPACITY] = {false};
    if (path_reaches(model, destination, source, visited)) return false;

    int existing = circuit_model_wire_for_input(model, destination,
                                                destination_input);
    if (existing >= 0) {
        model->wires[existing].source = source;
        circuit_model_evaluate(model);
        return true;
    }
    for (size_t i = 0; i < CIRCUIT_WIRE_CAPACITY; ++i) {
        if (model->wires[i].used) continue;
        model->wires[i] = (circuit_wire_t){
            .used = true,
            .source = source,
            .destination = destination,
            .destination_input = destination_input,
        };
        circuit_model_evaluate(model);
        return true;
    }
    return false;
}

bool circuit_model_set_type(circuit_model_t *model, uint8_t node,
                            circuit_gate_type_t type) {
    if (!valid_node(model, node) || type >= CIRCUIT_GATE_COUNT) return false;
    circuit_node_t *changed = &model->nodes[node];
    if (changed->type == type) return true;
    changed->type = type;
    assign_label(model, changed);
    size_t inputs = circuit_gate_input_count(type);
    for (size_t i = 0; i < CIRCUIT_WIRE_CAPACITY; ++i) {
        circuit_wire_t *wire = &model->wires[i];
        if (!wire->used) continue;
        if ((wire->destination == node && wire->destination_input >= inputs) ||
            (wire->source == node && !circuit_gate_has_output(type))) {
            wire->used = false;
        }
    }
    circuit_model_evaluate(model);
    return true;
}

bool circuit_model_toggle_input(circuit_model_t *model, uint8_t node) {
    if (!valid_node(model, node) ||
        model->nodes[node].type != CIRCUIT_GATE_INPUT) {
        return false;
    }
    model->nodes[node].input_value = !model->nodes[node].input_value;
    circuit_model_evaluate(model);
    return true;
}

static bool evaluate_node(circuit_model_t *model, uint8_t node,
                          uint8_t *state, bool *value) {
    if (!valid_node(model, node)) return false;
    if (state[node] == 2u) {
        *value = model->nodes[node].output_value;
        return true;
    }
    if (state[node] == 1u) {
        model->cycle_detected = true;
        *value = false;
        return false;
    }
    state[node] = 1u;
    circuit_node_t *current = &model->nodes[node];
    bool inputs[2] = {false, false};
    bool valid = true;
    size_t input_count = circuit_gate_input_count(current->type);
    for (size_t input = 0; input < input_count; ++input) {
        int wire_index = circuit_model_wire_for_input(
            model, node, (uint8_t)input);
        if (wire_index >= 0) {
            valid = evaluate_node(model,
                                  model->wires[wire_index].source,
                                  state, &inputs[input]) && valid;
        }
    }

    bool output = false;
    switch (current->type) {
        case CIRCUIT_GATE_INPUT: output = current->input_value; break;
        case CIRCUIT_GATE_OUTPUT: output = inputs[0]; break;
        case CIRCUIT_GATE_NOT: output = !inputs[0]; break;
        case CIRCUIT_GATE_AND: output = inputs[0] && inputs[1]; break;
        case CIRCUIT_GATE_OR: output = inputs[0] || inputs[1]; break;
        case CIRCUIT_GATE_XOR: output = inputs[0] != inputs[1]; break;
        case CIRCUIT_GATE_NAND: output = !(inputs[0] && inputs[1]); break;
        case CIRCUIT_GATE_NOR: output = !(inputs[0] || inputs[1]); break;
        case CIRCUIT_GATE_IMPLIES: output = !inputs[0] || inputs[1]; break;
        case CIRCUIT_GATE_XNOR: output = inputs[0] == inputs[1]; break;
        case CIRCUIT_GATE_COUNT: valid = false; break;
    }
    current->output_value = valid && output;
    state[node] = 2u;
    *value = current->output_value;
    return valid;
}

bool circuit_model_evaluate(circuit_model_t *model) {
    if (!model) return false;
    uint8_t state[CIRCUIT_NODE_CAPACITY] = {0};
    model->cycle_detected = false;
    bool valid = true;
    for (uint8_t node = 0; node < CIRCUIT_NODE_CAPACITY; ++node) {
        if (!model->nodes[node].used || state[node] == 2u) continue;
        bool value = false;
        valid = evaluate_node(model, node, state, &value) && valid;
    }
    return valid && !model->cycle_detected;
}

void circuit_model_init(circuit_model_t *model) {
    circuit_model_clear(model);
    int input_a = circuit_model_add(model, CIRCUIT_GATE_INPUT, 24, 72);
    int input_b = circuit_model_add(model, CIRCUIT_GATE_INPUT, 24, 198);
    int gate = circuit_model_add(model, CIRCUIT_GATE_AND, 190, 136);
    int output = circuit_model_add(model, CIRCUIT_GATE_OUTPUT, 358, 136);
    if (input_a >= 0 && input_b >= 0 && gate >= 0 && output >= 0) {
        (void)circuit_model_connect(model, (uint8_t)input_a,
                                    (uint8_t)gate, 0u);
        (void)circuit_model_connect(model, (uint8_t)input_b,
                                    (uint8_t)gate, 1u);
        (void)circuit_model_connect(model, (uint8_t)gate,
                                    (uint8_t)output, 0u);
    }
    circuit_model_evaluate(model);
}
