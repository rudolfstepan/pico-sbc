#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PROGRAMMER_BIN = 2,
    PROGRAMMER_DEC = 10,
    PROGRAMMER_HEX = 16
} programmer_base_t;

typedef enum {
    PROGRAMMER_OP_NONE,
    PROGRAMMER_OP_AND,
    PROGRAMMER_OP_OR,
    PROGRAMMER_OP_XOR
} programmer_binary_op_t;

typedef enum {
    PROGRAMMER_INPUT_OK,
    PROGRAMMER_INPUT_INVALID_DIGIT,
    PROGRAMMER_INPUT_OVERFLOW
} programmer_input_status_t;

typedef struct {
    uint64_t value;
    uint64_t left_value;
    programmer_base_t base;
    programmer_binary_op_t pending_op;
    char input[65];
    size_t input_len;
    bool replace_input;
    unsigned int word_bits;
    unsigned int selected_bit;
    bool signed_mode;
    bool carry;
    bool overflow;
} programmer_engine_t;

void programmer_engine_init(programmer_engine_t *engine);
void programmer_engine_clear(programmer_engine_t *engine);
void programmer_engine_delete(programmer_engine_t *engine);
void programmer_engine_set_base(programmer_engine_t *engine, programmer_base_t base);
bool programmer_engine_set_word_bits(programmer_engine_t *engine,
                                     unsigned int bits);
void programmer_engine_set_value(programmer_engine_t *engine, uint64_t value);
void programmer_engine_toggle_signed(programmer_engine_t *engine);
void programmer_engine_select_bit(programmer_engine_t *engine, int offset);
programmer_input_status_t programmer_engine_append(programmer_engine_t *engine,
                                                    char digit);
void programmer_engine_set_binary_op(programmer_engine_t *engine,
                                     programmer_binary_op_t op);
bool programmer_engine_equals(programmer_engine_t *engine);
void programmer_engine_shift_left(programmer_engine_t *engine);
void programmer_engine_shift_right(programmer_engine_t *engine);
void programmer_engine_not(programmer_engine_t *engine);
void programmer_engine_negate(programmer_engine_t *engine);
void programmer_engine_rotate_left(programmer_engine_t *engine);
void programmer_engine_rotate_right(programmer_engine_t *engine);
void programmer_engine_swap_endian(programmer_engine_t *engine);
void programmer_engine_increment(programmer_engine_t *engine);
void programmer_engine_decrement(programmer_engine_t *engine);
void programmer_engine_set_selected_bit(programmer_engine_t *engine);
void programmer_engine_clear_selected_bit(programmer_engine_t *engine);
void programmer_engine_toggle_selected_bit(programmer_engine_t *engine);
bool programmer_engine_selected_bit(const programmer_engine_t *engine);
void programmer_engine_clear_flags(programmer_engine_t *engine);
void programmer_engine_format(uint64_t value, programmer_base_t base,
                              char *buffer, size_t buffer_size);
const char *programmer_engine_base_name(programmer_base_t base);
const char *programmer_engine_op_name(programmer_binary_op_t op);
