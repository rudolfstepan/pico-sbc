#include "programmer_engine.h"

#include "number_formats.h"

#include <limits.h>
#include <string.h>

static int digit_value(char digit) {
    if (digit >= '0' && digit <= '9') return digit - '0';
    if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
    if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
    return -1;
}

void programmer_engine_format(uint64_t value, programmer_base_t base,
                              char *buffer, size_t buffer_size) {
    static const char digits[] = "0123456789ABCDEF";
    char reversed[65];
    size_t length = 0;

    if (!buffer || buffer_size == 0) return;
    do {
        reversed[length++] = digits[value % (uint64_t)base];
        value /= (uint64_t)base;
    } while (value && length < sizeof reversed);

    size_t output_length = length < buffer_size - 1 ? length : buffer_size - 1;
    for (size_t i = 0; i < output_length; ++i) {
        buffer[i] = reversed[length - i - 1];
    }
    buffer[output_length] = '\0';
}

static void refresh_input(programmer_engine_t *engine) {
    programmer_engine_format(engine->value, engine->base,
                             engine->input, sizeof engine->input);
    engine->input_len = strlen(engine->input);
}

static void finish_operation(programmer_engine_t *engine, uint64_t value) {
    engine->value = number_format_apply_width(value, engine->word_bits);
    engine->replace_input = true;
    refresh_input(engine);
}

void programmer_engine_init(programmer_engine_t *engine) {
    memset(engine, 0, sizeof *engine);
    engine->base = PROGRAMMER_DEC;
    engine->word_bits = 64;
    engine->input[0] = '0';
    engine->input[1] = '\0';
    engine->input_len = 1;
    engine->replace_input = true;
}

void programmer_engine_clear(programmer_engine_t *engine) {
    programmer_base_t base = engine->base;
    unsigned int word_bits = engine->word_bits;
    bool signed_mode = engine->signed_mode;
    programmer_engine_init(engine);
    engine->base = base;
    engine->word_bits = word_bits;
    engine->signed_mode = signed_mode;
}

void programmer_engine_delete(programmer_engine_t *engine) {
    if (engine->replace_input) return;
    engine->value /= (uint64_t)engine->base;
    refresh_input(engine);
}

void programmer_engine_set_base(programmer_engine_t *engine, programmer_base_t base) {
    engine->base = base;
    refresh_input(engine);
}

bool programmer_engine_set_word_bits(programmer_engine_t *engine,
                                     unsigned int bits) {
    if (bits != 8 && bits != 16 && bits != 32 && bits != 64) return false;
    engine->word_bits = bits;
    if (engine->selected_bit >= bits) engine->selected_bit = bits - 1u;
    engine->value = number_format_apply_width(engine->value, bits);
    engine->left_value = number_format_apply_width(engine->left_value, bits);
    engine->carry = false;
    engine->overflow = false;
    refresh_input(engine);
    return true;
}

void programmer_engine_set_value(programmer_engine_t *engine, uint64_t value) {
    engine->carry = false;
    engine->overflow = false;
    finish_operation(engine, value);
}

void programmer_engine_toggle_signed(programmer_engine_t *engine) {
    engine->signed_mode = !engine->signed_mode;
}

void programmer_engine_select_bit(programmer_engine_t *engine, int offset) {
    int selected = (int)engine->selected_bit + offset;
    if (selected < 0) selected = 0;
    if (selected >= (int)engine->word_bits) selected = (int)engine->word_bits - 1;
    engine->selected_bit = (unsigned int)selected;
}

programmer_input_status_t programmer_engine_append(programmer_engine_t *engine,
                                                    char digit) {
    int value = digit_value(digit);
    if (value < 0 || value >= (int)engine->base) {
        return PROGRAMMER_INPUT_INVALID_DIGIT;
    }

    uint64_t current = engine->replace_input ? 0 : engine->value;
    uint64_t maximum = number_format_mask(engine->word_bits);
    if (current > (maximum - (uint64_t)value) / (uint64_t)engine->base) {
        engine->overflow = true;
        return PROGRAMMER_INPUT_OVERFLOW;
    }

    engine->value = current * (uint64_t)engine->base + (uint64_t)value;
    engine->replace_input = false;
    engine->carry = false;
    engine->overflow = false;
    refresh_input(engine);
    return PROGRAMMER_INPUT_OK;
}

void programmer_engine_set_binary_op(programmer_engine_t *engine,
                                     programmer_binary_op_t op) {
    engine->left_value = engine->value;
    engine->pending_op = op;
    engine->value = 0;
    engine->replace_input = true;
    refresh_input(engine);
}

bool programmer_engine_equals(programmer_engine_t *engine) {
    switch (engine->pending_op) {
        case PROGRAMMER_OP_AND:
            engine->value = engine->left_value & engine->value;
            break;
        case PROGRAMMER_OP_OR:
            engine->value = engine->left_value | engine->value;
            break;
        case PROGRAMMER_OP_XOR:
            engine->value = engine->left_value ^ engine->value;
            break;
        case PROGRAMMER_OP_NONE:
            return false;
    }
    engine->value = number_format_apply_width(engine->value, engine->word_bits);
    engine->carry = false;
    engine->overflow = false;
    engine->pending_op = PROGRAMMER_OP_NONE;
    engine->replace_input = true;
    refresh_input(engine);
    return true;
}

void programmer_engine_shift_left(programmer_engine_t *engine) {
    engine->carry = number_format_test_bit(engine->value, engine->word_bits,
                                           engine->word_bits - 1u);
    engine->overflow = false;
    finish_operation(engine, engine->value << 1u);
}

void programmer_engine_shift_right(programmer_engine_t *engine) {
    engine->carry = (engine->value & 1u) != 0;
    engine->overflow = false;
    finish_operation(engine, engine->value >> 1u);
}

void programmer_engine_not(programmer_engine_t *engine) {
    engine->carry = false;
    engine->overflow = false;
    finish_operation(engine, ~engine->value);
}

void programmer_engine_negate(programmer_engine_t *engine) {
    uint64_t sign = UINT64_C(1) << (engine->word_bits - 1u);
    engine->carry = engine->value != 0;
    engine->overflow = number_format_apply_width(engine->value,
                                                  engine->word_bits) == sign;
    finish_operation(engine, ~engine->value + 1u);
}

void programmer_engine_rotate_left(programmer_engine_t *engine) {
    engine->carry = number_format_test_bit(engine->value, engine->word_bits,
                                           engine->word_bits - 1u);
    engine->overflow = false;
    finish_operation(engine, number_format_rotate_left(
        engine->value, engine->word_bits, 1));
}

void programmer_engine_rotate_right(programmer_engine_t *engine) {
    engine->carry = (engine->value & 1u) != 0;
    engine->overflow = false;
    finish_operation(engine, number_format_rotate_right(
        engine->value, engine->word_bits, 1));
}

void programmer_engine_swap_endian(programmer_engine_t *engine) {
    engine->carry = false;
    engine->overflow = false;
    finish_operation(engine, number_format_swap_endian(
        engine->value, engine->word_bits));
}

void programmer_engine_increment(programmer_engine_t *engine) {
    uint64_t value = number_format_apply_width(engine->value,
                                                engine->word_bits);
    uint64_t sign = UINT64_C(1) << (engine->word_bits - 1u);
    engine->carry = value == number_format_mask(engine->word_bits);
    engine->overflow = value == sign - 1u;
    finish_operation(engine, value + 1u);
}

void programmer_engine_decrement(programmer_engine_t *engine) {
    uint64_t value = number_format_apply_width(engine->value,
                                                engine->word_bits);
    uint64_t sign = UINT64_C(1) << (engine->word_bits - 1u);
    engine->carry = value != 0;
    engine->overflow = value == sign;
    finish_operation(engine, value - 1u);
}

void programmer_engine_set_selected_bit(programmer_engine_t *engine) {
    engine->carry = false;
    engine->overflow = false;
    finish_operation(engine, number_format_set_bit(
        engine->value, engine->word_bits, engine->selected_bit));
}

void programmer_engine_clear_selected_bit(programmer_engine_t *engine) {
    engine->carry = false;
    engine->overflow = false;
    finish_operation(engine, number_format_clear_bit(
        engine->value, engine->word_bits, engine->selected_bit));
}

void programmer_engine_toggle_selected_bit(programmer_engine_t *engine) {
    engine->carry = false;
    engine->overflow = false;
    finish_operation(engine, number_format_toggle_bit(
        engine->value, engine->word_bits, engine->selected_bit));
}

bool programmer_engine_selected_bit(const programmer_engine_t *engine) {
    return number_format_test_bit(engine->value, engine->word_bits,
                                  engine->selected_bit);
}

void programmer_engine_clear_flags(programmer_engine_t *engine) {
    engine->carry = false;
    engine->overflow = false;
}

const char *programmer_engine_base_name(programmer_base_t base) {
    switch (base) {
        case PROGRAMMER_BIN: return "BIN";
        case PROGRAMMER_DEC: return "DEC";
        case PROGRAMMER_HEX: return "HEX";
        default: return "?";
    }
}

const char *programmer_engine_op_name(programmer_binary_op_t op) {
    switch (op) {
        case PROGRAMMER_OP_AND: return "AND";
        case PROGRAMMER_OP_OR: return "OR";
        case PROGRAMMER_OP_XOR: return "XOR";
        case PROGRAMMER_OP_NONE: return "";
        default: return "";
    }
}
