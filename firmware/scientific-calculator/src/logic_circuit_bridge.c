#include "logic_circuit_bridge.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define BRIDGE_INVALID_NODE UINT8_MAX
#define BRIDGE_LEFT_MARGIN 36
#define BRIDGE_RIGHT_MARGIN 36
#define BRIDGE_TOP_MARGIN 54
#define BRIDGE_BOTTOM_MARGIN 54
#define BRIDGE_NODE_WIDTH 88
#define BRIDGE_NODE_HEIGHT 54
#define BRIDGE_MAX_X_STEP 120
#define BRIDGE_Y_PITCH 96

typedef struct {
    const logic_program_t *program;
    circuit_model_t model;
    uint8_t mapped[LOGIC_NODE_CAPACITY];
    uint8_t variable_nodes[LOGIC_VARIABLE_COUNT];
    uint8_t constant_nodes[2];
    uint8_t depth[CIRCUIT_NODE_CAPACITY];
    uint8_t state[LOGIC_NODE_CAPACITY];
    uint8_t assignment;
    logic_circuit_status_t status;
} program_builder_t;

typedef struct {
    const circuit_model_t *model;
    char *output;
    size_t capacity;
    size_t length;
    uint8_t input_nodes[LOGIC_VARIABLE_COUNT];
    uint8_t assignment;
    bool visiting[CIRCUIT_NODE_CAPACITY];
    logic_circuit_status_t status;
} expression_builder_t;

static bool valid_circuit_node(const circuit_model_t *model, uint8_t node) {
    return model && node < CIRCUIT_NODE_CAPACITY && model->nodes[node].used;
}

static circuit_gate_type_t gate_for_logic(logic_node_kind_t kind) {
    switch (kind) {
        case LOGIC_NODE_NOT: return CIRCUIT_GATE_NOT;
        case LOGIC_NODE_AND: return CIRCUIT_GATE_AND;
        case LOGIC_NODE_OR: return CIRCUIT_GATE_OR;
        case LOGIC_NODE_XOR: return CIRCUIT_GATE_XOR;
        case LOGIC_NODE_NAND: return CIRCUIT_GATE_NAND;
        case LOGIC_NODE_NOR: return CIRCUIT_GATE_NOR;
        case LOGIC_NODE_IMPLIES: return CIRCUIT_GATE_IMPLIES;
        case LOGIC_NODE_XNOR: return CIRCUIT_GATE_XNOR;
        default: return CIRCUIT_GATE_COUNT;
    }
}

static uint8_t add_node(program_builder_t *builder, circuit_gate_type_t type) {
    int added = circuit_model_add(&builder->model, type, 0, 0);
    if (added < 0) {
        builder->status = LOGIC_CIRCUIT_TOO_COMPLEX;
        return BRIDGE_INVALID_NODE;
    }
    return (uint8_t)added;
}

static uint8_t build_program_node(program_builder_t *builder, uint8_t index) {
    if (builder->status != LOGIC_CIRCUIT_OK) return BRIDGE_INVALID_NODE;
    if (index >= builder->program->node_count ||
        index >= LOGIC_NODE_CAPACITY) {
        builder->status = LOGIC_CIRCUIT_INVALID;
        return BRIDGE_INVALID_NODE;
    }
    if (builder->state[index] == 2u) return builder->mapped[index];
    if (builder->state[index] == 1u) {
        builder->status = LOGIC_CIRCUIT_CYCLE;
        return BRIDGE_INVALID_NODE;
    }
    builder->state[index] = 1u;

    const logic_node_t *source = &builder->program->nodes[index];
    uint8_t mapped = BRIDGE_INVALID_NODE;
    uint8_t depth = 0u;
    if (source->kind == LOGIC_NODE_VARIABLE) {
        if (source->value >= LOGIC_VARIABLE_COUNT) {
            builder->status = LOGIC_CIRCUIT_INVALID;
            return BRIDGE_INVALID_NODE;
        }
        mapped = builder->variable_nodes[source->value];
        if (mapped == BRIDGE_INVALID_NODE) {
            mapped = add_node(builder, CIRCUIT_GATE_INPUT);
            if (mapped == BRIDGE_INVALID_NODE) return mapped;
            builder->variable_nodes[source->value] = mapped;
            circuit_node_t *input = &builder->model.nodes[mapped];
            snprintf(input->label, sizeof input->label, "%c",
                     (int)('A' + source->value));
            input->input_value =
                (builder->assignment & (1u << source->value)) != 0u;
        }
    } else if (source->kind == LOGIC_NODE_CONSTANT) {
        uint8_t value = source->value ? 1u : 0u;
        mapped = builder->constant_nodes[value];
        if (mapped == BRIDGE_INVALID_NODE) {
            mapped = add_node(builder, CIRCUIT_GATE_INPUT);
            if (mapped == BRIDGE_INVALID_NODE) return mapped;
            builder->constant_nodes[value] = mapped;
            circuit_node_t *constant = &builder->model.nodes[mapped];
            snprintf(constant->label, sizeof constant->label, "%u",
                     (unsigned int)value);
            constant->input_value = value != 0u;
        }
    } else {
        circuit_gate_type_t type = gate_for_logic(source->kind);
        if (type == CIRCUIT_GATE_COUNT) {
            builder->status = LOGIC_CIRCUIT_INVALID;
            return BRIDGE_INVALID_NODE;
        }
        uint8_t left = build_program_node(builder, source->left);
        if (left == BRIDGE_INVALID_NODE) return left;
        uint8_t right = BRIDGE_INVALID_NODE;
        if (source->kind != LOGIC_NODE_NOT) {
            right = build_program_node(builder, source->right);
            if (right == BRIDGE_INVALID_NODE) return right;
        }
        mapped = add_node(builder, type);
        if (mapped == BRIDGE_INVALID_NODE) return mapped;
        if (!circuit_model_connect(&builder->model, left, mapped, 0u) ||
            (source->kind != LOGIC_NODE_NOT &&
             !circuit_model_connect(&builder->model, right, mapped, 1u))) {
            builder->status = LOGIC_CIRCUIT_INVALID;
            return BRIDGE_INVALID_NODE;
        }
        depth = (uint8_t)(builder->depth[left] + 1u);
        if (source->kind != LOGIC_NODE_NOT &&
            builder->depth[right] + 1u > depth) {
            depth = (uint8_t)(builder->depth[right] + 1u);
        }
    }

    builder->mapped[index] = mapped;
    builder->depth[mapped] = depth;
    builder->state[index] = 2u;
    return mapped;
}

static void layout_model(program_builder_t *builder, uint8_t output) {
    uint8_t max_depth = builder->depth[output];
    size_t level_count[CIRCUIT_NODE_CAPACITY] = {0};
    for (size_t node = 0; node < CIRCUIT_NODE_CAPACITY; ++node) {
        if (!builder->model.nodes[node].used) continue;
        uint8_t depth = builder->depth[node];
        level_count[depth]++;
        if (depth > max_depth) max_depth = depth;
    }

    int left = BRIDGE_LEFT_MARGIN;
    int right = CIRCUIT_WORLD_WIDTH - BRIDGE_RIGHT_MARGIN -
                BRIDGE_NODE_WIDTH;
    int x_step = max_depth ? (right - left) / max_depth : 0;
    if (x_step > BRIDGE_MAX_X_STEP) x_step = BRIDGE_MAX_X_STEP;
    size_t level_slot[CIRCUIT_NODE_CAPACITY] = {0};
    for (size_t node = 0; node < CIRCUIT_NODE_CAPACITY; ++node) {
        circuit_node_t *item = &builder->model.nodes[node];
        if (!item->used) continue;
        uint8_t depth = builder->depth[node];
        size_t count = level_count[depth];
        int available_y = CIRCUIT_WORLD_HEIGHT - BRIDGE_TOP_MARGIN -
                          BRIDGE_BOTTOM_MARGIN - BRIDGE_NODE_HEIGHT;
        int pitch = count > 1u
            ? available_y / (int)(count - 1u) : 0;
        if (pitch > BRIDGE_Y_PITCH) pitch = BRIDGE_Y_PITCH;
        item->x = (int16_t)(left + depth * x_step);
        item->y = (int16_t)(BRIDGE_TOP_MARGIN +
                            (int)level_slot[depth] * pitch);
        level_slot[depth]++;
    }
}

logic_circuit_status_t logic_circuit_from_program(
    const logic_program_t *program, uint8_t assignment,
    circuit_model_t *model) {
    if (!program || !model || !program->node_count) {
        return LOGIC_CIRCUIT_EMPTY;
    }
    if (program->node_count > LOGIC_NODE_CAPACITY ||
        program->root >= program->node_count) {
        return LOGIC_CIRCUIT_INVALID;
    }
    program_builder_t builder = {
        .program = program,
        .assignment = assignment,
        .status = LOGIC_CIRCUIT_OK,
    };
    memset(builder.mapped, BRIDGE_INVALID_NODE, sizeof builder.mapped);
    memset(builder.variable_nodes, BRIDGE_INVALID_NODE,
           sizeof builder.variable_nodes);
    memset(builder.constant_nodes, BRIDGE_INVALID_NODE,
           sizeof builder.constant_nodes);
    circuit_model_clear(&builder.model);

    uint8_t root = build_program_node(&builder, program->root);
    if (root == BRIDGE_INVALID_NODE) return builder.status;
    uint8_t output = add_node(&builder, CIRCUIT_GATE_OUTPUT);
    if (output == BRIDGE_INVALID_NODE) return builder.status;
    builder.depth[output] = (uint8_t)(builder.depth[root] + 1u);
    if (!circuit_model_connect(&builder.model, root, output, 0u)) {
        return LOGIC_CIRCUIT_INVALID;
    }
    layout_model(&builder, output);
    if (!circuit_model_evaluate(&builder.model)) {
        return LOGIC_CIRCUIT_INVALID;
    }
    *model = builder.model;
    return LOGIC_CIRCUIT_OK;
}

static void append_text(expression_builder_t *builder, const char *text) {
    if (builder->status != LOGIC_CIRCUIT_OK) return;
    size_t length = strlen(text);
    if (length >= builder->capacity - builder->length) {
        builder->status = LOGIC_CIRCUIT_OUTPUT_FULL;
        return;
    }
    memcpy(builder->output + builder->length, text, length);
    builder->length += length;
    builder->output[builder->length] = '\0';
}

static int source_for_input(const circuit_model_t *model,
                            uint8_t destination, uint8_t input) {
    int wire = circuit_model_wire_for_input(model, destination, input);
    if (wire < 0) return -1;
    const circuit_wire_t *connection = &model->wires[wire];
    return valid_circuit_node(model, connection->source)
        ? connection->source : -1;
}

static const char *operator_for_gate(circuit_gate_type_t type) {
    switch (type) {
        case CIRCUIT_GATE_AND: return "&";
        case CIRCUIT_GATE_OR: return "|";
        case CIRCUIT_GATE_XOR: return "^";
        case CIRCUIT_GATE_NAND: return " NAND ";
        case CIRCUIT_GATE_NOR: return " NOR ";
        case CIRCUIT_GATE_IMPLIES: return " IMPLIES ";
        case CIRCUIT_GATE_XNOR: return " XNOR ";
        default: return NULL;
    }
}

static void write_circuit_node(expression_builder_t *builder, uint8_t node) {
    if (builder->status != LOGIC_CIRCUIT_OK) return;
    if (!valid_circuit_node(builder->model, node)) {
        builder->status = LOGIC_CIRCUIT_INVALID;
        return;
    }
    if (builder->visiting[node]) {
        builder->status = LOGIC_CIRCUIT_CYCLE;
        return;
    }
    builder->visiting[node] = true;
    const circuit_node_t *current = &builder->model->nodes[node];
    if (current->type == CIRCUIT_GATE_INPUT) {
        if ((strcmp(current->label, "0") == 0 ||
             strcmp(current->label, "1") == 0)) {
            append_text(builder, current->input_value ? "1" : "0");
        } else if (current->label[0] >= 'A' &&
                   current->label[0] <
                       'A' + (int)LOGIC_VARIABLE_COUNT &&
                   current->label[1] == '\0') {
            uint8_t variable = (uint8_t)(current->label[0] - 'A');
            uint8_t previous = builder->input_nodes[variable];
            if (previous != BRIDGE_INVALID_NODE && previous != node) {
                builder->status = LOGIC_CIRCUIT_DUPLICATE_INPUT;
            } else {
                builder->input_nodes[variable] = node;
                if (current->input_value) {
                    builder->assignment |= (uint8_t)(1u << variable);
                }
                char label[2] = {current->label[0], '\0'};
                append_text(builder, label);
            }
        } else {
            builder->status = LOGIC_CIRCUIT_INVALID_INPUT;
        }
    } else if (current->type == CIRCUIT_GATE_OUTPUT) {
        int source = source_for_input(builder->model, node, 0u);
        if (source < 0) append_text(builder, "0");
        else write_circuit_node(builder, (uint8_t)source);
    } else if (current->type == CIRCUIT_GATE_NOT) {
        append_text(builder, "!(");
        int source = source_for_input(builder->model, node, 0u);
        if (source < 0) append_text(builder, "0");
        else write_circuit_node(builder, (uint8_t)source);
        append_text(builder, ")");
    } else {
        const char *operation = operator_for_gate(current->type);
        if (!operation) {
            builder->status = LOGIC_CIRCUIT_INVALID;
        } else {
            append_text(builder, "(");
            int left = source_for_input(builder->model, node, 0u);
            int right = source_for_input(builder->model, node, 1u);
            if (left < 0) append_text(builder, "0");
            else write_circuit_node(builder, (uint8_t)left);
            append_text(builder, operation);
            if (right < 0) append_text(builder, "0");
            else write_circuit_node(builder, (uint8_t)right);
            append_text(builder, ")");
        }
    }
    builder->visiting[node] = false;
}

logic_circuit_status_t logic_circuit_to_expression(
    const circuit_model_t *model, uint8_t target,
    char *expression, size_t expression_size,
    uint8_t *assignment) {
    if (!model || !expression || expression_size == 0u) {
        return LOGIC_CIRCUIT_INVALID;
    }
    expression[0] = '\0';
    if (target == CIRCUIT_NODE_NONE) {
        for (uint8_t node = 0; node < CIRCUIT_NODE_CAPACITY; ++node) {
            if (model->nodes[node].used &&
                model->nodes[node].type == CIRCUIT_GATE_OUTPUT) {
                target = node;
                break;
            }
        }
    }
    if (!valid_circuit_node(model, target)) {
        return LOGIC_CIRCUIT_NO_OUTPUT;
    }

    expression_builder_t builder = {
        .model = model,
        .output = expression,
        .capacity = expression_size,
        .status = LOGIC_CIRCUIT_OK,
    };
    memset(builder.input_nodes, BRIDGE_INVALID_NODE,
           sizeof builder.input_nodes);
    write_circuit_node(&builder, target);
    if (builder.status != LOGIC_CIRCUIT_OK) {
        expression[0] = '\0';
        return builder.status;
    }
    if (assignment) *assignment = builder.assignment;
    return LOGIC_CIRCUIT_OK;
}

const char *logic_circuit_status_text(logic_circuit_status_t status) {
    switch (status) {
        case LOGIC_CIRCUIT_OK: return "OK";
        case LOGIC_CIRCUIT_EMPTY: return "EMPTY LOGIC";
        case LOGIC_CIRCUIT_TOO_COMPLEX: return "PLAN TOO LARGE";
        case LOGIC_CIRCUIT_NO_OUTPUT: return "NO OUTPUT";
        case LOGIC_CIRCUIT_INVALID_INPUT: return "INPUT LABEL A-F";
        case LOGIC_CIRCUIT_DUPLICATE_INPUT: return "DUPLICATE INPUT";
        case LOGIC_CIRCUIT_CYCLE: return "CIRCUIT CYCLE";
        case LOGIC_CIRCUIT_OUTPUT_FULL: return "EXPR TOO LONG";
        case LOGIC_CIRCUIT_INVALID:
        default: return "INVALID CIRCUIT";
    }
}
