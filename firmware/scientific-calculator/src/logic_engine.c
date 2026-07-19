#include "logic_engine.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define INVALID_NODE UINT8_MAX
#define IMPLICANT_CAPACITY 256u

typedef enum {
    TOKEN_END,
    TOKEN_VARIABLE,
    TOKEN_CONSTANT,
    TOKEN_NOT,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_XOR,
    TOKEN_NAND,
    TOKEN_NOR,
    TOKEN_XNOR,
    TOKEN_LEFT,
    TOKEN_RIGHT,
    TOKEN_INVALID
} token_kind_t;

typedef struct {
    const char *expression;
    const char *cursor;
    const char *token_start;
    token_kind_t token;
    uint8_t value;
    logic_program_t *program;
    logic_status_t status;
    int error_position;
} parser_t;

typedef struct {
    uint8_t bits;
    uint8_t mask;
    uint64_t covered;
    bool used;
} implicant_t;

typedef struct {
    char *output;
    size_t capacity;
    size_t length;
    bool valid;
} output_writer_t;

static bool identifier_equals(const char *text, size_t length,
                              const char *expected) {
    if (strlen(expected) != length) return false;
    for (size_t i = 0; i < length; ++i) {
        if (toupper((unsigned char)text[i]) != expected[i]) return false;
    }
    return true;
}

static void next_token(parser_t *parser) {
    while (isspace((unsigned char)*parser->cursor)) parser->cursor++;
    parser->token_start = parser->cursor;
    char current = *parser->cursor;
    if (!current) {
        parser->token = TOKEN_END;
        return;
    }
    parser->cursor++;
    switch (current) {
        case '!': case '~': parser->token = TOKEN_NOT; return;
        case '&': case '*': parser->token = TOKEN_AND; return;
        case '|': case '+': parser->token = TOKEN_OR; return;
        case '^': parser->token = TOKEN_XOR; return;
        case '(': parser->token = TOKEN_LEFT; return;
        case ')': parser->token = TOKEN_RIGHT; return;
        case '0': case '1':
            parser->token = TOKEN_CONSTANT;
            parser->value = (uint8_t)(current - '0');
            return;
        default: break;
    }
    if (!isalpha((unsigned char)current)) {
        parser->token = TOKEN_INVALID;
        return;
    }
    while (isalnum((unsigned char)*parser->cursor) ||
           *parser->cursor == '_') {
        parser->cursor++;
    }
    size_t length = (size_t)(parser->cursor - parser->token_start);
    if (length == 1) {
        char variable = (char)toupper((unsigned char)current);
        const char variable_limit = (char)('A' + LOGIC_VARIABLE_COUNT);
        if (variable >= 'A' && variable < variable_limit) {
            parser->token = TOKEN_VARIABLE;
            parser->value = (uint8_t)(variable - 'A');
            return;
        }
    }
    if (identifier_equals(parser->token_start, length, "NOT")) {
        parser->token = TOKEN_NOT;
    } else if (identifier_equals(parser->token_start, length, "AND")) {
        parser->token = TOKEN_AND;
    } else if (identifier_equals(parser->token_start, length, "OR")) {
        parser->token = TOKEN_OR;
    } else if (identifier_equals(parser->token_start, length, "XOR")) {
        parser->token = TOKEN_XOR;
    } else if (identifier_equals(parser->token_start, length, "NAND")) {
        parser->token = TOKEN_NAND;
    } else if (identifier_equals(parser->token_start, length, "NOR")) {
        parser->token = TOKEN_NOR;
    } else if (identifier_equals(parser->token_start, length, "XNOR")) {
        parser->token = TOKEN_XNOR;
    } else {
        parser->token = TOKEN_INVALID;
    }
}

static uint8_t add_node(parser_t *parser, logic_node_kind_t kind,
                        uint8_t left, uint8_t right, uint8_t value) {
    if (parser->program->node_count >= LOGIC_NODE_CAPACITY) {
        parser->status = LOGIC_STATUS_TOO_COMPLEX;
        parser->error_position = (int)(parser->token_start -
                                       parser->expression) + 1;
        return INVALID_NODE;
    }
    size_t index = parser->program->node_count++;
    parser->program->nodes[index] = (logic_node_t){
        .kind = kind,
        .left = left,
        .right = right,
        .value = value,
    };
    return (uint8_t)index;
}

static uint8_t parse_or(parser_t *parser);

static void syntax_error(parser_t *parser) {
    if (parser->status != LOGIC_STATUS_OK) return;
    parser->status = LOGIC_STATUS_SYNTAX;
    parser->error_position = (int)(parser->token_start -
                                   parser->expression) + 1;
}

static uint8_t parse_primary(parser_t *parser) {
    if (parser->token == TOKEN_VARIABLE) {
        uint8_t variable = parser->value;
        parser->program->variable_mask |= (uint8_t)(1u << variable);
        next_token(parser);
        return add_node(parser, LOGIC_NODE_VARIABLE, INVALID_NODE,
                        INVALID_NODE, variable);
    }
    if (parser->token == TOKEN_CONSTANT) {
        uint8_t value = parser->value;
        next_token(parser);
        return add_node(parser, LOGIC_NODE_CONSTANT, INVALID_NODE,
                        INVALID_NODE, value);
    }
    if (parser->token == TOKEN_LEFT) {
        next_token(parser);
        uint8_t expression = parse_or(parser);
        if (parser->token != TOKEN_RIGHT) {
            syntax_error(parser);
            return INVALID_NODE;
        }
        next_token(parser);
        return expression;
    }
    syntax_error(parser);
    return INVALID_NODE;
}

static uint8_t parse_unary(parser_t *parser) {
    if (parser->token != TOKEN_NOT) return parse_primary(parser);
    next_token(parser);
    uint8_t operand = parse_unary(parser);
    if (operand == INVALID_NODE) return INVALID_NODE;
    return add_node(parser, LOGIC_NODE_NOT, operand, INVALID_NODE, 0);
}

static logic_node_kind_t binary_kind(token_kind_t token) {
    switch (token) {
        case TOKEN_AND: return LOGIC_NODE_AND;
        case TOKEN_OR: return LOGIC_NODE_OR;
        case TOKEN_XOR: return LOGIC_NODE_XOR;
        case TOKEN_NAND: return LOGIC_NODE_NAND;
        case TOKEN_NOR: return LOGIC_NODE_NOR;
        case TOKEN_XNOR: return LOGIC_NODE_XNOR;
        default: return LOGIC_NODE_CONSTANT;
    }
}

static uint8_t parse_and(parser_t *parser) {
    uint8_t left = parse_unary(parser);
    while (parser->status == LOGIC_STATUS_OK &&
           (parser->token == TOKEN_AND || parser->token == TOKEN_NAND)) {
        token_kind_t operation = parser->token;
        next_token(parser);
        uint8_t right = parse_unary(parser);
        if (right == INVALID_NODE) return INVALID_NODE;
        left = add_node(parser, binary_kind(operation), left, right, 0);
    }
    return left;
}

static uint8_t parse_xor(parser_t *parser) {
    uint8_t left = parse_and(parser);
    while (parser->status == LOGIC_STATUS_OK &&
           (parser->token == TOKEN_XOR || parser->token == TOKEN_XNOR)) {
        token_kind_t operation = parser->token;
        next_token(parser);
        uint8_t right = parse_and(parser);
        if (right == INVALID_NODE) return INVALID_NODE;
        left = add_node(parser, binary_kind(operation), left, right, 0);
    }
    return left;
}

static uint8_t parse_or(parser_t *parser) {
    uint8_t left = parse_xor(parser);
    while (parser->status == LOGIC_STATUS_OK &&
           (parser->token == TOKEN_OR || parser->token == TOKEN_NOR)) {
        token_kind_t operation = parser->token;
        next_token(parser);
        uint8_t right = parse_xor(parser);
        if (right == INVALID_NODE) return INVALID_NODE;
        left = add_node(parser, binary_kind(operation), left, right, 0);
    }
    return left;
}

logic_status_t logic_engine_compile(const char *expression,
                                    logic_program_t *program,
                                    int *error_position) {
    if (error_position) *error_position = 0;
    if (!expression || !*expression) return LOGIC_STATUS_EMPTY;
    memset(program, 0, sizeof *program);
    parser_t parser = {
        .expression = expression,
        .cursor = expression,
        .program = program,
        .status = LOGIC_STATUS_OK,
    };
    next_token(&parser);
    program->root = parse_or(&parser);
    if (parser.status == LOGIC_STATUS_OK && parser.token != TOKEN_END) {
        syntax_error(&parser);
    }
    if (parser.status != LOGIC_STATUS_OK || program->root == INVALID_NODE) {
        if (error_position) *error_position = parser.error_position;
        memset(program, 0, sizeof *program);
        return parser.status == LOGIC_STATUS_OK
            ? LOGIC_STATUS_SYNTAX : parser.status;
    }
    return LOGIC_STATUS_OK;
}

static bool evaluate_node(const logic_program_t *program, uint8_t node,
                          uint8_t assignment) {
    const logic_node_t *current = &program->nodes[node];
    if (current->kind == LOGIC_NODE_CONSTANT) return current->value != 0;
    if (current->kind == LOGIC_NODE_VARIABLE) {
        return (assignment & (1u << current->value)) != 0;
    }
    bool left = evaluate_node(program, current->left, assignment);
    if (current->kind == LOGIC_NODE_NOT) return !left;
    bool right = evaluate_node(program, current->right, assignment);
    switch (current->kind) {
        case LOGIC_NODE_AND: return left && right;
        case LOGIC_NODE_OR: return left || right;
        case LOGIC_NODE_XOR: return left != right;
        case LOGIC_NODE_NAND: return !(left && right);
        case LOGIC_NODE_NOR: return !(left || right);
        case LOGIC_NODE_XNOR: return left == right;
        default: return false;
    }
}

bool logic_engine_evaluate(const logic_program_t *program,
                           uint8_t assignment) {
    return program && program->node_count &&
           evaluate_node(program, program->root, assignment);
}

size_t logic_engine_variable_count(const logic_program_t *program) {
    uint8_t mask = program ? program->variable_mask : 0;
    size_t count = 0;
    while (mask) {
        count += mask & 1u;
        mask >>= 1;
    }
    return count;
}

size_t logic_engine_truth_row_count(const logic_program_t *program) {
    return (size_t)1u << logic_engine_variable_count(program);
}

uint8_t logic_engine_assignment_for_row(const logic_program_t *program,
                                        size_t row) {
    uint8_t assignment = 0;
    unsigned int compact_bit = 0;
    for (unsigned int variable = 0; variable < LOGIC_VARIABLE_COUNT;
         ++variable) {
        if (!(program->variable_mask & (1u << variable))) continue;
        if (row & ((size_t)1u << compact_bit)) {
            assignment |= (uint8_t)(1u << variable);
        }
        compact_bit++;
    }
    return assignment;
}

static void output_append(output_writer_t *writer, const char *text) {
    size_t length = strlen(text);
    if (!writer->valid || length >= writer->capacity - writer->length) {
        writer->valid = false;
        return;
    }
    memcpy(writer->output + writer->length, text, length);
    writer->length += length;
    writer->output[writer->length] = '\0';
}

static void output_char(output_writer_t *writer, char value) {
    char text[2] = {value, '\0'};
    output_append(writer, text);
}

static void format_assignment(output_writer_t *writer,
                              const logic_program_t *program,
                              uint8_t assignment, bool dnf) {
    output_char(writer, '(');
    bool first = true;
    for (unsigned int variable = 0; variable < LOGIC_VARIABLE_COUNT;
         ++variable) {
        if (!(program->variable_mask & (1u << variable))) continue;
        if (!first) output_char(writer, dnf ? '&' : '|');
        bool value = (assignment & (1u << variable)) != 0;
        if ((dnf && !value) || (!dnf && value)) output_char(writer, '!');
        output_char(writer, (char)('A' + variable));
        first = false;
    }
    output_char(writer, ')');
}

logic_status_t logic_engine_format_canonical(
    const logic_program_t *program, bool dnf,
    char *output, size_t output_size) {
    if (!program || !program->node_count || !output || output_size == 0) {
        return LOGIC_STATUS_EMPTY;
    }
    output_writer_t writer = {
        .output = output,
        .capacity = output_size,
        .valid = true,
    };
    output[0] = '\0';
    size_t rows = logic_engine_truth_row_count(program);
    bool first = true;
    for (size_t row = 0; row < rows; ++row) {
        uint8_t assignment = logic_engine_assignment_for_row(program, row);
        bool value = logic_engine_evaluate(program, assignment);
        if (value != dnf) continue;
        if (!first) output_char(&writer, dnf ? '|' : '&');
        if (program->variable_mask) {
            format_assignment(&writer, program, assignment, dnf);
        } else {
            output_char(&writer, value ? '1' : '0');
        }
        first = false;
    }
    if (first) output_char(&writer, dnf ? '0' : '1');
    return writer.valid ? LOGIC_STATUS_OK : LOGIC_STATUS_OUTPUT_FULL;
}

static unsigned int bit_count8(uint8_t value) {
    unsigned int count = 0;
    while (value) {
        count += value & 1u;
        value >>= 1;
    }
    return count;
}

static bool add_unique_implicant(implicant_t *items, size_t *count,
                                 implicant_t candidate) {
    for (size_t i = 0; i < *count; ++i) {
        if (items[i].bits == candidate.bits &&
            items[i].mask == candidate.mask) {
            items[i].covered |= candidate.covered;
            return true;
        }
    }
    if (*count >= IMPLICANT_CAPACITY) return false;
    items[(*count)++] = candidate;
    return true;
}

static bool implicant_covers(const implicant_t *implicant,
                             uint8_t assignment) {
    return ((assignment ^ implicant->bits) & (uint8_t)~implicant->mask) == 0;
}

static logic_status_t collect_prime_implicants(
    const logic_program_t *program, bool target_value,
    implicant_t *primes, size_t *prime_count, uint64_t *target_rows) {
    /* Static: ~12 KB combined would overflow the 2 KB RP2040 stack.
     * The firmware is single-threaded, so this is safe. */
    static implicant_t current[IMPLICANT_CAPACITY];
    static implicant_t next[IMPLICANT_CAPACITY];
    size_t current_count = 0;
    *prime_count = 0;
    *target_rows = 0;
    size_t rows = logic_engine_truth_row_count(program);
    for (size_t row = 0; row < rows; ++row) {
        uint8_t assignment = logic_engine_assignment_for_row(program, row);
        if (logic_engine_evaluate(program, assignment) != target_value) continue;
        *target_rows |= UINT64_C(1) << row;
        current[current_count++] = (implicant_t){
            .bits = assignment,
            .covered = UINT64_C(1) << row,
        };
    }

    while (current_count) {
        for (size_t i = 0; i < current_count; ++i) current[i].used = false;
        size_t next_count = 0;
        for (size_t i = 0; i < current_count; ++i) {
            for (size_t j = i + 1; j < current_count; ++j) {
                if (current[i].mask != current[j].mask) continue;
                uint8_t difference = (current[i].bits ^ current[j].bits) &
                                     (uint8_t)~current[i].mask;
                if (bit_count8(difference) != 1) continue;
                current[i].used = true;
                current[j].used = true;
                implicant_t combined = {
                    .bits = current[i].bits & (uint8_t)~difference,
                    .mask = current[i].mask | difference,
                    .covered = current[i].covered | current[j].covered,
                };
                if (!add_unique_implicant(next, &next_count, combined)) {
                    return LOGIC_STATUS_TOO_COMPLEX;
                }
            }
        }
        for (size_t i = 0; i < current_count; ++i) {
            if (!current[i].used &&
                !add_unique_implicant(primes, prime_count, current[i])) {
                return LOGIC_STATUS_TOO_COMPLEX;
            }
        }
        memcpy(current, next, next_count * sizeof next[0]);
        current_count = next_count;
    }
    return LOGIC_STATUS_OK;
}

static uint64_t implicant_rows(const logic_program_t *program,
                               const implicant_t *implicant,
                               uint64_t target_rows) {
    uint64_t covered = 0;
    size_t rows = logic_engine_truth_row_count(program);
    for (size_t row = 0; row < rows; ++row) {
        if (!(target_rows & (UINT64_C(1) << row))) continue;
        uint8_t assignment = logic_engine_assignment_for_row(program, row);
        if (implicant_covers(implicant, assignment)) {
            covered |= UINT64_C(1) << row;
        }
    }
    return covered;
}

static void format_implicant(output_writer_t *writer,
                             const logic_program_t *program,
                             const implicant_t *implicant, bool dnf) {
    output_char(writer, '(');
    bool first = true;
    for (unsigned int variable = 0; variable < LOGIC_VARIABLE_COUNT;
         ++variable) {
        uint8_t bit = (uint8_t)(1u << variable);
        if (!(program->variable_mask & bit) || (implicant->mask & bit)) continue;
        if (!first) output_char(writer, dnf ? '&' : '|');
        bool value = (implicant->bits & bit) != 0;
        if ((dnf && !value) || (!dnf && value)) output_char(writer, '!');
        output_char(writer, (char)('A' + variable));
        first = false;
    }
    if (first) output_char(writer, dnf ? '1' : '0');
    output_char(writer, ')');
}

logic_status_t logic_engine_format_simplified(
    const logic_program_t *program, bool dnf,
    char *output, size_t output_size) {
    if (!program || !program->node_count || !output || output_size == 0) {
        return LOGIC_STATUS_EMPTY;
    }
    /* Static: too large for the 2 KB RP2040 stack (single-threaded). */
    static implicant_t primes[IMPLICANT_CAPACITY];
    size_t prime_count = 0;
    uint64_t target_rows = 0;
    logic_status_t status = collect_prime_implicants(
        program, dnf, primes, &prime_count, &target_rows);
    if (status != LOGIC_STATUS_OK) return status;

    size_t row_count = logic_engine_truth_row_count(program);
    uint64_t all_rows = row_count == 64
        ? UINT64_MAX : (UINT64_C(1) << row_count) - 1u;
    if (!target_rows || target_rows == all_rows) {
        snprintf(output, output_size, "%c",
                 !target_rows ? (dnf ? '0' : '1') : (dnf ? '1' : '0'));
        return output_size >= 2 ? LOGIC_STATUS_OK : LOGIC_STATUS_OUTPUT_FULL;
    }

    static uint64_t prime_rows[IMPLICANT_CAPACITY];
    static bool selected[IMPLICANT_CAPACITY];
    memset(selected, 0, sizeof selected);
    for (size_t i = 0; i < prime_count; ++i) {
        prime_rows[i] = implicant_rows(program, &primes[i], target_rows);
    }
    uint64_t covered = 0;
    for (size_t row = 0; row < row_count; ++row) {
        uint64_t row_bit = UINT64_C(1) << row;
        if (!(target_rows & row_bit)) continue;
        size_t only = 0;
        size_t matches = 0;
        for (size_t prime = 0; prime < prime_count; ++prime) {
            if (prime_rows[prime] & row_bit) {
                only = prime;
                matches++;
            }
        }
        if (matches == 1) selected[only] = true;
    }
    for (size_t i = 0; i < prime_count; ++i) {
        if (selected[i]) covered |= prime_rows[i];
    }
    while ((covered & target_rows) != target_rows) {
        size_t best = prime_count;
        unsigned int best_count = 0;
        for (size_t i = 0; i < prime_count; ++i) {
            if (selected[i]) continue;
            unsigned int count = (unsigned int)__builtin_popcountll(
                prime_rows[i] & target_rows & ~covered);
            if (count > best_count) {
                best = i;
                best_count = count;
            }
        }
        if (best == prime_count) return LOGIC_STATUS_TOO_COMPLEX;
        selected[best] = true;
        covered |= prime_rows[best];
    }

    output_writer_t writer = {
        .output = output,
        .capacity = output_size,
        .valid = true,
    };
    output[0] = '\0';
    bool first = true;
    for (size_t i = 0; i < prime_count; ++i) {
        if (!selected[i]) continue;
        if (!first) output_char(&writer, dnf ? '|' : '&');
        format_implicant(&writer, program, &primes[i], dnf);
        first = false;
    }
    return writer.valid ? LOGIC_STATUS_OK : LOGIC_STATUS_OUTPUT_FULL;
}

const char *logic_engine_status_text(logic_status_t status) {
    switch (status) {
        case LOGIC_STATUS_OK: return "OK";
        case LOGIC_STATUS_EMPTY: return "EMPTY";
        case LOGIC_STATUS_SYNTAX: return "SYNTAX";
        case LOGIC_STATUS_TOO_COMPLEX: return "TOO COMPLEX";
        case LOGIC_STATUS_OUTPUT_FULL: return "OUTPUT FULL";
        default: return "LOGIC ERROR";
    }
}
