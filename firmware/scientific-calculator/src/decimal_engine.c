#include "decimal_engine.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DECIMAL_MAX_EXPONENT 4096
#define DECIMAL_MAX_POWER 1000
#define DECIMAL_MAX_DEPTH 6u
#define DECIMAL_MAX_DIVISION_STEPS (DECIMAL_ENGINE_MAX_DIGITS * 2u)
#define WORK_DIGITS (DECIMAL_ENGINE_MAX_DIGITS + 1u)
#define PARSER_VALUE_COUNT 32u

typedef struct {
    uint8_t digits[DECIMAL_ENGINE_MAX_DIGITS];
    size_t count;
    int scale;
    bool negative;
    bool inexact;
} decimal_value_t;

typedef struct {
    uint8_t digits[WORK_DIGITS];
    size_t count;
} unsigned_work_t;

typedef struct {
    const char *start;
    const char *cursor;
    const char *ans_text;
    decimal_status_t status;
    int error_position;
    unsigned int depth;
    size_t value_count;
} parser_t;

typedef struct {
    decimal_value_t add_values[2];
    uint16_t product[DECIMAL_ENGINE_MAX_DIGITS * 2u];
    decimal_value_t division_values[2];
    unsigned_work_t division_remainder;
    unsigned_work_t division_twice;
    decimal_value_t modulo_values[3];
    unsigned_work_t modulo_remainder;
    decimal_value_t power_values[5];
    decimal_value_t parser_values[PARSER_VALUE_COUNT];
} decimal_workspace_t;

static decimal_workspace_t workspace;

static void decimal_zero(decimal_value_t *value) {
    memset(value, 0, sizeof *value);
}

static void trim_high(decimal_value_t *value) {
    while (value->count && value->digits[value->count - 1u] == 0u) {
        value->count--;
    }
    if (!value->count) value->negative = false;
}

static void decimal_normalize(decimal_value_t *value) {
    trim_high(value);
    while (value->count && value->digits[0] == 0u) {
        memmove(&value->digits[0], &value->digits[1], value->count - 1u);
        value->digits[--value->count] = 0u;
        value->scale--;
    }
    if (!value->count) {
        value->scale = 0;
        value->negative = false;
    }
}

static bool append_digit(decimal_value_t *value, unsigned int digit) {
    if (!value->count && digit == 0u) return true;
    if (value->count >= DECIMAL_ENGINE_MAX_DIGITS) return false;
    memmove(&value->digits[1], &value->digits[0], value->count);
    value->digits[0] = (uint8_t)digit;
    value->count++;
    return true;
}

static decimal_status_t parse_literal(const char *text,
                                      decimal_value_t *value) {
    decimal_zero(value);
    if (!text || !*text) return DECIMAL_STATUS_SYNTAX;
    const char *cursor = text;
    if (*cursor == '+' || *cursor == '-') {
        value->negative = *cursor++ == '-';
    }
    bool saw_digit = false;
    bool after_point = false;
    int fractional_digits = 0;
    while (*cursor) {
        if (isdigit((unsigned char)*cursor)) {
            saw_digit = true;
            if (!append_digit(value, (unsigned int)(*cursor - '0'))) {
                return DECIMAL_STATUS_PRECISION;
            }
            if (after_point) fractional_digits++;
            cursor++;
            continue;
        }
        if (*cursor == '.' && !after_point) {
            after_point = true;
            cursor++;
            continue;
        }
        break;
    }
    if (!saw_digit) return DECIMAL_STATUS_SYNTAX;

    int exponent = 0;
    if (*cursor == 'e' || *cursor == 'E') {
        cursor++;
        bool exponent_negative = false;
        if (*cursor == '+' || *cursor == '-') {
            exponent_negative = *cursor++ == '-';
        }
        if (!isdigit((unsigned char)*cursor)) return DECIMAL_STATUS_SYNTAX;
        while (isdigit((unsigned char)*cursor)) {
            if (exponent > DECIMAL_MAX_EXPONENT) {
                return DECIMAL_STATUS_RANGE;
            }
            exponent = exponent * 10 + (*cursor++ - '0');
        }
        if (exponent_negative) exponent = -exponent;
    }
    if (*cursor) return DECIMAL_STATUS_SYNTAX;
    if (fractional_digits - exponent > DECIMAL_MAX_EXPONENT ||
        fractional_digits - exponent < -DECIMAL_MAX_EXPONENT) {
        return DECIMAL_STATUS_RANGE;
    }
    value->scale = fractional_digits - exponent;
    decimal_normalize(value);
    return DECIMAL_STATUS_OK;
}

static bool align_value(const decimal_value_t *source, int scale,
                        decimal_value_t *aligned) {
    decimal_zero(aligned);
    if (!source->count) return true;
    int difference = scale - source->scale;
    if (difference < 0 ||
        source->count + (size_t)difference > DECIMAL_ENGINE_MAX_DIGITS) {
        return false;
    }
    memcpy(&aligned->digits[difference], source->digits, source->count);
    aligned->count = source->count + (size_t)difference;
    aligned->scale = scale;
    aligned->negative = source->negative;
    aligned->inexact = source->inexact;
    return true;
}

static int compare_unsigned(const decimal_value_t *first,
                            const decimal_value_t *second) {
    if (first->count != second->count) {
        return first->count < second->count ? -1 : 1;
    }
    for (size_t i = first->count; i > 0; --i) {
        if (first->digits[i - 1u] != second->digits[i - 1u]) {
            return first->digits[i - 1u] < second->digits[i - 1u] ? -1 : 1;
        }
    }
    return 0;
}

static bool add_unsigned(const decimal_value_t *first,
                         const decimal_value_t *second,
                         decimal_value_t *result) {
    decimal_zero(result);
    size_t count = first->count > second->count
        ? first->count : second->count;
    unsigned int carry = 0;
    for (size_t i = 0; i < count || carry; ++i) {
        if (i >= DECIMAL_ENGINE_MAX_DIGITS) return false;
        unsigned int total = carry;
        if (i < first->count) total += first->digits[i];
        if (i < second->count) total += second->digits[i];
        result->digits[i] = (uint8_t)(total % 10u);
        carry = total / 10u;
        result->count = i + 1u;
    }
    if (result->count < count) result->count = count;
    return true;
}

static void subtract_unsigned(const decimal_value_t *first,
                              const decimal_value_t *second,
                              decimal_value_t *result) {
    decimal_zero(result);
    int borrow = 0;
    for (size_t i = 0; i < first->count; ++i) {
        int digit = (int)first->digits[i] - borrow -
            (i < second->count ? (int)second->digits[i] : 0);
        if (digit < 0) {
            digit += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result->digits[i] = (uint8_t)digit;
    }
    result->count = first->count;
    trim_high(result);
}

static decimal_status_t decimal_add(const decimal_value_t *first,
                                    const decimal_value_t *second,
                                    decimal_value_t *result) {
    int scale = first->scale > second->scale ? first->scale : second->scale;
    decimal_value_t *aligned_first = &workspace.add_values[0];
    decimal_value_t *aligned_second = &workspace.add_values[1];
    if (!align_value(first, scale, aligned_first) ||
        !align_value(second, scale, aligned_second)) {
        return DECIMAL_STATUS_PRECISION;
    }
    bool negative = false;
    if (aligned_first->negative == aligned_second->negative) {
        if (!add_unsigned(aligned_first, aligned_second, result)) {
            return DECIMAL_STATUS_PRECISION;
        }
        negative = aligned_first->negative;
    } else {
        int comparison = compare_unsigned(aligned_first, aligned_second);
        if (comparison >= 0) {
            subtract_unsigned(aligned_first, aligned_second, result);
            negative = aligned_first->negative;
        } else {
            subtract_unsigned(aligned_second, aligned_first, result);
            negative = aligned_second->negative;
        }
    }
    result->scale = scale;
    result->negative = negative && result->count;
    result->inexact = first->inexact || second->inexact;
    decimal_normalize(result);
    return DECIMAL_STATUS_OK;
}

static decimal_status_t decimal_subtract(const decimal_value_t *first,
                                         const decimal_value_t *second,
                                         decimal_value_t *result) {
    decimal_value_t negated = *second;
    if (negated.count) negated.negative = !negated.negative;
    return decimal_add(first, &negated, result);
}

static decimal_status_t decimal_multiply(const decimal_value_t *first,
                                         const decimal_value_t *second,
                                         decimal_value_t *result) {
    decimal_zero(result);
    if (!first->count || !second->count) {
        result->inexact = first->inexact || second->inexact;
        return DECIMAL_STATUS_OK;
    }
    uint16_t *product = workspace.product;
    memset(product, 0, sizeof workspace.product);
    for (size_t i = 0; i < first->count; ++i) {
        for (size_t j = 0; j < second->count; ++j) {
            product[i + j] +=
                (uint16_t)(first->digits[i] * second->digits[j]);
        }
    }
    for (size_t i = 0; i + 1u <
         sizeof workspace.product / sizeof workspace.product[0]; ++i) {
        product[i + 1u] += product[i] / 10u;
        product[i] %= 10u;
    }
    size_t count = sizeof workspace.product / sizeof workspace.product[0];
    while (count && product[count - 1u] == 0u) count--;
    if (count > DECIMAL_ENGINE_MAX_DIGITS) {
        return DECIMAL_STATUS_PRECISION;
    }
    for (size_t i = 0; i < count; ++i) {
        result->digits[i] = (uint8_t)product[i];
    }
    result->count = count;
    result->scale = first->scale + second->scale;
    if (result->scale > DECIMAL_MAX_EXPONENT ||
        result->scale < -DECIMAL_MAX_EXPONENT) {
        return DECIMAL_STATUS_RANGE;
    }
    result->negative = first->negative != second->negative;
    result->inexact = first->inexact || second->inexact;
    decimal_normalize(result);
    return DECIMAL_STATUS_OK;
}

static void work_trim(unsigned_work_t *value) {
    while (value->count && value->digits[value->count - 1u] == 0u) {
        value->count--;
    }
}

static int work_compare_decimal(const unsigned_work_t *first,
                                const decimal_value_t *second) {
    if (first->count != second->count) {
        return first->count < second->count ? -1 : 1;
    }
    for (size_t i = first->count; i > 0; --i) {
        if (first->digits[i - 1u] != second->digits[i - 1u]) {
            return first->digits[i - 1u] < second->digits[i - 1u] ? -1 : 1;
        }
    }
    return 0;
}

static void work_subtract_decimal(unsigned_work_t *first,
                                  const decimal_value_t *second) {
    int borrow = 0;
    for (size_t i = 0; i < first->count; ++i) {
        int digit = (int)first->digits[i] - borrow -
            (i < second->count ? (int)second->digits[i] : 0);
        if (digit < 0) {
            digit += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        first->digits[i] = (uint8_t)digit;
    }
    work_trim(first);
}

static bool work_append_digit(unsigned_work_t *value, unsigned int digit) {
    if (!value->count && digit == 0u) return true;
    if (value->count >= WORK_DIGITS) return false;
    memmove(&value->digits[1], &value->digits[0], value->count);
    value->digits[0] = (uint8_t)digit;
    value->count++;
    return true;
}

static void unsigned_divide(const decimal_value_t *numerator,
                            const decimal_value_t *denominator,
                            decimal_value_t *quotient,
                            unsigned_work_t *remainder) {
    decimal_zero(quotient);
    memset(remainder, 0, sizeof *remainder);
    quotient->count = numerator->count;
    for (size_t i = numerator->count; i > 0; --i) {
        (void)work_append_digit(remainder, numerator->digits[i - 1u]);
        unsigned int digit = 0;
        while (work_compare_decimal(remainder, denominator) >= 0) {
            work_subtract_decimal(remainder, denominator);
            digit++;
        }
        quotient->digits[i - 1u] = (uint8_t)digit;
    }
    trim_high(quotient);
}

static bool work_double(unsigned_work_t *value) {
    unsigned int carry = 0;
    for (size_t i = 0; i < value->count; ++i) {
        unsigned int digit = value->digits[i] * 2u + carry;
        value->digits[i] = (uint8_t)(digit % 10u);
        carry = digit / 10u;
    }
    if (carry) {
        if (value->count >= WORK_DIGITS) return false;
        value->digits[value->count++] = (uint8_t)carry;
    }
    return true;
}

static bool increment_unsigned(decimal_value_t *value) {
    size_t i = 0;
    unsigned int carry = 1;
    while (carry && i < value->count) {
        unsigned int digit = value->digits[i] + carry;
        value->digits[i] = (uint8_t)(digit % 10u);
        carry = digit / 10u;
        i++;
    }
    if (carry) {
        if (value->count >= DECIMAL_ENGINE_MAX_DIGITS) return false;
        value->digits[value->count++] = 1u;
    }
    return true;
}

static decimal_status_t decimal_divide(const decimal_value_t *first,
                                       const decimal_value_t *second,
                                       decimal_value_t *result) {
    if (!second->count) return DECIMAL_STATUS_DIV_ZERO;
    if (!first->count) {
        decimal_zero(result);
        result->inexact = first->inexact || second->inexact;
        return DECIMAL_STATUS_OK;
    }
    decimal_value_t *numerator = &workspace.division_values[0];
    decimal_value_t *denominator = &workspace.division_values[1];
    unsigned_work_t *remainder = &workspace.division_remainder;
    *numerator = *first;
    *denominator = *second;
    numerator->negative = false;
    denominator->negative = false;
    unsigned_divide(numerator, denominator, result, remainder);
    size_t fractional_digits = 0;
    while (remainder->count &&
           fractional_digits < DECIMAL_MAX_DIVISION_STEPS &&
           result->count < DECIMAL_ENGINE_MAX_DIGITS) {
        if (!work_append_digit(remainder, 0u)) {
            return DECIMAL_STATUS_PRECISION;
        }
        unsigned int digit = 0;
        while (work_compare_decimal(remainder, denominator) >= 0) {
            work_subtract_decimal(remainder, denominator);
            digit++;
        }
        if (!append_digit(result, digit)) break;
        fractional_digits++;
    }
    bool rounded = remainder->count != 0u;
    int rounding_scale_adjustment = 0;
    if (rounded) {
        unsigned_work_t *twice = &workspace.division_twice;
        *twice = *remainder;
        if (!work_double(twice)) return DECIMAL_STATUS_PRECISION;
        int comparison = work_compare_decimal(twice, denominator);
        bool round_up = comparison > 0 ||
            (comparison == 0 && result->count && (result->digits[0] & 1u));
        if (round_up && !increment_unsigned(result)) {
            decimal_zero(result);
            result->digits[0] = 1u;
            result->count = 1u;
            rounding_scale_adjustment = -(int)DECIMAL_ENGINE_MAX_DIGITS;
        }
    }
    result->scale = (int)fractional_digits + first->scale - second->scale +
        rounding_scale_adjustment;
    if (result->scale > DECIMAL_MAX_EXPONENT ||
        result->scale < -DECIMAL_MAX_EXPONENT) {
        return DECIMAL_STATUS_RANGE;
    }
    result->negative = first->negative != second->negative;
    result->inexact = first->inexact || second->inexact || rounded;
    decimal_normalize(result);
    return DECIMAL_STATUS_OK;
}

static decimal_status_t decimal_modulo(const decimal_value_t *first,
                                       const decimal_value_t *second,
                                       decimal_value_t *result) {
    if (!second->count) return DECIMAL_STATUS_DIV_ZERO;
    int scale = first->scale > second->scale ? first->scale : second->scale;
    decimal_value_t *numerator = &workspace.modulo_values[0];
    decimal_value_t *denominator = &workspace.modulo_values[1];
    decimal_value_t *quotient = &workspace.modulo_values[2];
    if (!align_value(first, scale, numerator) ||
        !align_value(second, scale, denominator)) {
        return DECIMAL_STATUS_PRECISION;
    }
    numerator->negative = false;
    denominator->negative = false;
    unsigned_work_t *remainder = &workspace.modulo_remainder;
    unsigned_divide(numerator, denominator, quotient, remainder);
    decimal_zero(result);
    if (remainder->count > DECIMAL_ENGINE_MAX_DIGITS) {
        return DECIMAL_STATUS_PRECISION;
    }
    memcpy(result->digits, remainder->digits, remainder->count);
    result->count = remainder->count;
    result->scale = scale;
    result->negative = first->negative;
    result->inexact = first->inexact || second->inexact;
    decimal_normalize(result);
    return DECIMAL_STATUS_OK;
}

static bool decimal_to_integer(const decimal_value_t *value, int *integer) {
    if (value->inexact || value->scale > 0) return false;
    unsigned int magnitude = 0;
    for (size_t i = value->count; i > 0; --i) {
        if (magnitude > (unsigned int)DECIMAL_MAX_POWER / 10u) return false;
        magnitude = magnitude * 10u + value->digits[i - 1u];
    }
    for (int i = 0; i < -value->scale; ++i) {
        if (magnitude > (unsigned int)DECIMAL_MAX_POWER / 10u) return false;
        magnitude *= 10u;
    }
    if (magnitude > DECIMAL_MAX_POWER) return false;
    *integer = value->negative ? -(int)magnitude : (int)magnitude;
    return true;
}

static void decimal_one(decimal_value_t *value) {
    decimal_zero(value);
    value->digits[0] = 1u;
    value->count = 1u;
}

static decimal_status_t decimal_power(const decimal_value_t *base,
                                      const decimal_value_t *power,
                                      decimal_value_t *result) {
    int exponent = 0;
    if (!decimal_to_integer(power, &exponent)) {
        return DECIMAL_STATUS_UNSUPPORTED;
    }
    bool reciprocal = exponent < 0;
    unsigned int remaining = reciprocal
        ? (unsigned int)(-exponent) : (unsigned int)exponent;
    decimal_value_t *factor = &workspace.power_values[0];
    decimal_value_t *product = &workspace.power_values[1];
    decimal_value_t *square = &workspace.power_values[2];
    decimal_value_t *one = &workspace.power_values[3];
    decimal_value_t *quotient = &workspace.power_values[4];
    *factor = *base;
    decimal_one(result);
    while (remaining) {
        if (remaining & 1u) {
            decimal_status_t status = decimal_multiply(result, factor,
                                                        product);
            if (status != DECIMAL_STATUS_OK) return status;
            *result = *product;
        }
        remaining >>= 1u;
        if (remaining) {
            decimal_status_t status = decimal_multiply(factor, factor,
                                                        square);
            if (status != DECIMAL_STATUS_OK) return status;
            *factor = *square;
        }
    }
    if (reciprocal) {
        decimal_one(one);
        decimal_status_t status = decimal_divide(one, result, quotient);
        if (status != DECIMAL_STATUS_OK) return status;
        *result = *quotient;
    }
    result->inexact = result->inexact || base->inexact || power->inexact;
    return DECIMAL_STATUS_OK;
}

static void parser_skip_space(parser_t *parser) {
    while (isspace((unsigned char)*parser->cursor)) parser->cursor++;
}

static void parser_fail(parser_t *parser, decimal_status_t status) {
    if (parser->status != DECIMAL_STATUS_OK) return;
    parser->status = status;
    parser->error_position = (int)(parser->cursor - parser->start) + 1;
}

static decimal_value_t *parser_acquire_value(parser_t *parser) {
    if (parser->value_count >= PARSER_VALUE_COUNT) {
        parser_fail(parser, DECIMAL_STATUS_RANGE);
        return NULL;
    }
    return &workspace.parser_values[parser->value_count++];
}

static bool word_is_ans(const char *text) {
    return tolower((unsigned char)text[0]) == 'a' &&
           tolower((unsigned char)text[1]) == 'n' &&
           tolower((unsigned char)text[2]) == 's' &&
           !isalnum((unsigned char)text[3]) && text[3] != '_';
}

static decimal_status_t parse_expression(parser_t *parser,
                                         decimal_value_t *value);
static decimal_status_t parse_unary(parser_t *parser,
                                    decimal_value_t *value);

static decimal_status_t parse_primary(parser_t *parser,
                                      decimal_value_t *value) {
    parser_skip_space(parser);
    if (*parser->cursor == '(') {
        if (++parser->depth > DECIMAL_MAX_DEPTH) {
            parser_fail(parser, DECIMAL_STATUS_RANGE);
            return parser->status;
        }
        parser->cursor++;
        decimal_status_t status = parse_expression(parser, value);
        parser_skip_space(parser);
        if (status == DECIMAL_STATUS_OK && *parser->cursor != ')') {
            parser_fail(parser, DECIMAL_STATUS_SYNTAX);
            status = parser->status;
        }
        if (*parser->cursor == ')') parser->cursor++;
        parser->depth--;
        return status;
    }
    if (isalpha((unsigned char)*parser->cursor)) {
        if (!word_is_ans(parser->cursor)) {
            parser_fail(parser, DECIMAL_STATUS_UNSUPPORTED);
            return parser->status;
        }
        parser->cursor += 3;
        decimal_status_t status = parse_literal(
            parser->ans_text && *parser->ans_text ? parser->ans_text : "0",
            value);
        if (status != DECIMAL_STATUS_OK) parser_fail(parser, status);
        return parser->status;
    }

    const char *start = parser->cursor;
    bool saw_digit = false;
    while (isdigit((unsigned char)*parser->cursor)) {
        saw_digit = true;
        parser->cursor++;
    }
    if (*parser->cursor == '.') {
        parser->cursor++;
        while (isdigit((unsigned char)*parser->cursor)) {
            saw_digit = true;
            parser->cursor++;
        }
    }
    if (!saw_digit) {
        parser_fail(parser, DECIMAL_STATUS_SYNTAX);
        return parser->status;
    }
    if (*parser->cursor == 'e' || *parser->cursor == 'E') {
        parser->cursor++;
        if (*parser->cursor == '+' || *parser->cursor == '-') parser->cursor++;
        if (!isdigit((unsigned char)*parser->cursor)) {
            parser_fail(parser, DECIMAL_STATUS_SYNTAX);
            return parser->status;
        }
        while (isdigit((unsigned char)*parser->cursor)) parser->cursor++;
    }
    size_t length = (size_t)(parser->cursor - start);
    if (length >= DECIMAL_ENGINE_TEXT_CAPACITY) {
        parser_fail(parser, DECIMAL_STATUS_PRECISION);
        return parser->status;
    }
    char literal[DECIMAL_ENGINE_TEXT_CAPACITY];
    memcpy(literal, start, length);
    literal[length] = '\0';
    decimal_status_t status = parse_literal(literal, value);
    if (status != DECIMAL_STATUS_OK) parser_fail(parser, status);
    return parser->status;
}

static decimal_status_t parse_power(parser_t *parser,
                                    decimal_value_t *value) {
    decimal_status_t status = parse_primary(parser, value);
    if (status != DECIMAL_STATUS_OK) return status;
    parser_skip_space(parser);
    if (*parser->cursor == '^') {
        parser->cursor++;
        size_t value_mark = parser->value_count;
        decimal_value_t *exponent = parser_acquire_value(parser);
        decimal_value_t *powered = parser_acquire_value(parser);
        if (!exponent || !powered) return parser->status;
        status = parse_unary(parser, exponent);
        if (status != DECIMAL_STATUS_OK) return status;
        status = decimal_power(value, exponent, powered);
        if (status != DECIMAL_STATUS_OK) {
            parser_fail(parser, status);
            return parser->status;
        }
        *value = *powered;
        parser->value_count = value_mark;
    }
    return DECIMAL_STATUS_OK;
}

static decimal_status_t parse_unary(parser_t *parser,
                                    decimal_value_t *value) {
    parser_skip_space(parser);
    if (*parser->cursor == '+' || *parser->cursor == '-') {
        bool negative = *parser->cursor++ == '-';
        decimal_status_t status = parse_unary(parser, value);
        if (status == DECIMAL_STATUS_OK && negative && value->count) {
            value->negative = !value->negative;
        }
        return status;
    }
    return parse_power(parser, value);
}

static decimal_status_t parse_term(parser_t *parser,
                                   decimal_value_t *value) {
    decimal_status_t status = parse_unary(parser, value);
    while (status == DECIMAL_STATUS_OK) {
        parser_skip_space(parser);
        char operation = *parser->cursor;
        if (operation != '*' && operation != '/' && operation != '%') break;
        parser->cursor++;
        size_t value_mark = parser->value_count;
        decimal_value_t *right = parser_acquire_value(parser);
        decimal_value_t *combined = parser_acquire_value(parser);
        if (!right || !combined) return parser->status;
        status = parse_unary(parser, right);
        if (status != DECIMAL_STATUS_OK) break;
        if (operation == '*') {
            status = decimal_multiply(value, right, combined);
        } else if (operation == '/') {
            status = decimal_divide(value, right, combined);
        } else {
            status = decimal_modulo(value, right, combined);
        }
        if (status != DECIMAL_STATUS_OK) {
            parser_fail(parser, status);
            return parser->status;
        }
        *value = *combined;
        parser->value_count = value_mark;
    }
    return status;
}

static decimal_status_t parse_expression(parser_t *parser,
                                         decimal_value_t *value) {
    decimal_status_t status = parse_term(parser, value);
    while (status == DECIMAL_STATUS_OK) {
        parser_skip_space(parser);
        char operation = *parser->cursor;
        if (operation != '+' && operation != '-') break;
        parser->cursor++;
        size_t value_mark = parser->value_count;
        decimal_value_t *right = parser_acquire_value(parser);
        decimal_value_t *combined = parser_acquire_value(parser);
        if (!right || !combined) return parser->status;
        status = parse_term(parser, right);
        if (status != DECIMAL_STATUS_OK) break;
        status = operation == '+'
            ? decimal_add(value, right, combined)
            : decimal_subtract(value, right, combined);
        if (status != DECIMAL_STATUS_OK) {
            parser_fail(parser, status);
            return parser->status;
        }
        *value = *combined;
        parser->value_count = value_mark;
    }
    return status;
}

static decimal_status_t format_decimal(const decimal_value_t *value,
                                       char *output, size_t output_size) {
    if (!output || !output_size) return DECIMAL_STATUS_RANGE;
    if (!value->count) {
        snprintf(output, output_size, "0");
        return DECIMAL_STATUS_OK;
    }
    int point = (int)value->count - value->scale;
    size_t sign = value->negative ? 1u : 0u;
    size_t plain_length;
    if (point <= 0) {
        plain_length = sign + 2u + (size_t)(-point) + value->count;
    } else if ((size_t)point >= value->count) {
        plain_length = sign + (size_t)point;
    } else {
        plain_length = sign + value->count + 1u;
    }
    char *cursor = output;
    size_t remaining = output_size;
#define PUT_CHARACTER(character) do { \
    if (remaining <= 1u) return DECIMAL_STATUS_RANGE; \
    *cursor++ = (character); \
    remaining--; \
} while (0)
    if (plain_length < output_size) {
        if (value->negative) PUT_CHARACTER('-');
        if (point <= 0) {
            PUT_CHARACTER('0');
            PUT_CHARACTER('.');
            for (int i = 0; i < -point; ++i) PUT_CHARACTER('0');
            for (size_t i = value->count; i > 0; --i) {
                PUT_CHARACTER((char)('0' + value->digits[i - 1u]));
            }
        } else {
            for (int position = 0; position < point; ++position) {
                int source = (int)value->count - 1 - position;
                PUT_CHARACTER(source >= 0
                    ? (char)('0' + value->digits[source]) : '0');
            }
            if ((size_t)point < value->count) {
                PUT_CHARACTER('.');
                for (size_t i = value->count - (size_t)point; i > 0; --i) {
                    PUT_CHARACTER((char)('0' + value->digits[i - 1u]));
                }
            }
        }
        *cursor = '\0';
        return DECIMAL_STATUS_OK;
    }
    if (value->negative) PUT_CHARACTER('-');
    PUT_CHARACTER((char)('0' + value->digits[value->count - 1u]));
    if (value->count > 1u) {
        PUT_CHARACTER('.');
        for (size_t i = value->count - 1u; i > 0; --i) {
            PUT_CHARACTER((char)('0' + value->digits[i - 1u]));
        }
    }
    int exponent = (int)value->count - 1 - value->scale;
    int written = snprintf(cursor, remaining, "e%+d", exponent);
    if (written < 0 || (size_t)written >= remaining) {
        return DECIMAL_STATUS_RANGE;
    }
#undef PUT_CHARACTER
    return DECIMAL_STATUS_OK;
}

decimal_status_t decimal_engine_evaluate(const char *expression,
                                         const char *ans_text,
                                         decimal_result_t *result,
                                         int *error_position) {
    if (error_position) *error_position = 0;
    if (!expression || !result) return DECIMAL_STATUS_SYNTAX;
    parser_t parser = {
        .start = expression,
        .cursor = expression,
        .ans_text = ans_text,
        .status = DECIMAL_STATUS_OK,
    };
    decimal_value_t *value = parser_acquire_value(&parser);
    if (!value) return parser.status;
    decimal_status_t status = parse_expression(&parser, value);
    parser_skip_space(&parser);
    if (status == DECIMAL_STATUS_OK && *parser.cursor) {
        parser_fail(&parser, isalpha((unsigned char)*parser.cursor) ||
                    *parser.cursor == ','
            ? DECIMAL_STATUS_UNSUPPORTED : DECIMAL_STATUS_SYNTAX);
        status = parser.status;
    }
    if (status != DECIMAL_STATUS_OK) {
        if (error_position) *error_position = parser.error_position;
        return status;
    }
    status = format_decimal(value, result->text, sizeof result->text);
    if (status != DECIMAL_STATUS_OK) return status;
    errno = 0;
    char *end = NULL;
    result->approximation = strtod(result->text, &end);
    if (!end || *end || !isfinite(result->approximation) || errno == ERANGE) {
        return DECIMAL_STATUS_RANGE;
    }
    result->exact = !value->inexact;
    return DECIMAL_STATUS_OK;
}

bool decimal_engine_pack_text(const char *text, uint8_t *packed,
                              size_t packed_size) {
    if (!packed || packed_size < DECIMAL_ENGINE_PACKED_CAPACITY) return false;
    decimal_value_t value;
    if (parse_literal(text, &value) != DECIMAL_STATUS_OK ||
        value.scale < INT16_MIN || value.scale > INT16_MAX) {
        return false;
    }
    memset(packed, 0, DECIMAL_ENGINE_PACKED_CAPACITY);
    packed[0] = value.negative ? 1u : 0u;
    packed[1] = (uint8_t)value.count;
    uint16_t encoded_scale = (uint16_t)(int16_t)value.scale;
    packed[2] = (uint8_t)encoded_scale;
    packed[3] = (uint8_t)(encoded_scale >> 8);
    for (size_t i = 0; i < value.count; ++i) {
        unsigned int shift = (unsigned int)(i & 1u) * 4u;
        packed[4u + i / 2u] |= (uint8_t)(value.digits[i] << shift);
    }
    return true;
}

bool decimal_engine_unpack_text(const uint8_t *packed, size_t packed_size,
                                char *text, size_t text_size) {
    if (!packed || packed_size < DECIMAL_ENGINE_PACKED_CAPACITY ||
        packed[0] > 1u || packed[1] > DECIMAL_ENGINE_MAX_DIGITS) {
        return false;
    }
    decimal_value_t value;
    decimal_zero(&value);
    value.negative = packed[0] != 0u;
    value.count = packed[1];
    uint16_t encoded_scale = (uint16_t)packed[2] |
        (uint16_t)packed[3] << 8;
    value.scale = (int16_t)encoded_scale;
    for (size_t i = 0; i < value.count; ++i) {
        unsigned int shift = (unsigned int)(i & 1u) * 4u;
        uint8_t digit = (packed[4u + i / 2u] >> shift) & 0x0fu;
        if (digit > 9u) return false;
        value.digits[i] = digit;
    }
    if ((!value.count && (value.negative || value.scale != 0)) ||
        (value.count &&
         (value.digits[0] == 0u || value.digits[value.count - 1u] == 0u))) {
        return false;
    }
    return format_decimal(&value, text, text_size) == DECIMAL_STATUS_OK;
}

const char *decimal_status_text(decimal_status_t status) {
    switch (status) {
        case DECIMAL_STATUS_OK: return "OK";
        case DECIMAL_STATUS_UNSUPPORTED: return "UNSUPPORTED";
        case DECIMAL_STATUS_SYNTAX: return "SYNTAX";
        case DECIMAL_STATUS_DIV_ZERO: return "DIV ZERO";
        case DECIMAL_STATUS_PRECISION: return "PRECISION";
        case DECIMAL_STATUS_RANGE: return "RANGE";
        default: return "ERROR";
    }
}
