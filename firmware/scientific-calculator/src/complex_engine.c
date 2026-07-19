#include "complex_engine.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PI_VALUE 3.14159265358979323846

typedef struct {
    const char *expression;
    const char *cursor;
    bool degrees;
    complex_status_t status;
    int error_position;
} parser_t;

static complex_value_t parse_expression(parser_t *parser);

static complex_value_t value(double real, double imag) {
    return (complex_value_t){.real = real, .imag = imag};
}

static void skip_spaces(parser_t *parser) {
    while (isspace((unsigned char)*parser->cursor)) parser->cursor++;
}

static void set_error(parser_t *parser, complex_status_t status) {
    if (parser->status != COMPLEX_STATUS_OK) return;
    parser->status = status;
    parser->error_position = (int)(parser->cursor - parser->expression) + 1;
}

static bool finite_value(complex_value_t input) {
    return isfinite(input.real) && isfinite(input.imag);
}

static complex_value_t add(complex_value_t left, complex_value_t right) {
    return value(left.real + right.real, left.imag + right.imag);
}

static complex_value_t subtract(complex_value_t left,
                                complex_value_t right) {
    return value(left.real - right.real, left.imag - right.imag);
}

static complex_value_t multiply(complex_value_t left,
                                complex_value_t right) {
    return value(left.real * right.real - left.imag * right.imag,
                 left.real * right.imag + left.imag * right.real);
}

static complex_value_t divide(parser_t *parser, complex_value_t left,
                              complex_value_t right) {
    double denominator = right.real * right.real + right.imag * right.imag;
    if (denominator == 0.0) {
        set_error(parser, COMPLEX_STATUS_DIV_ZERO);
        return value(0.0, 0.0);
    }
    return value((left.real * right.real + left.imag * right.imag) /
                     denominator,
                 (left.imag * right.real - left.real * right.imag) /
                     denominator);
}

static complex_value_t exponential(complex_value_t input) {
    double scale = exp(input.real);
    return value(scale * cos(input.imag), scale * sin(input.imag));
}

static complex_value_t logarithm(parser_t *parser,
                                 complex_value_t input) {
    double magnitude = hypot(input.real, input.imag);
    if (magnitude == 0.0) {
        set_error(parser, COMPLEX_STATUS_DOMAIN);
        return value(0.0, 0.0);
    }
    double imag = input.imag == 0.0 ? 0.0 : input.imag;
    return value(log(magnitude), atan2(imag, input.real));
}

static complex_value_t power(parser_t *parser, complex_value_t base,
                             complex_value_t exponent) {
    if (base.real == 0.0 && base.imag == 0.0) {
        if (exponent.imag == 0.0 && exponent.real > 0.0) {
            return value(0.0, 0.0);
        }
        set_error(parser, COMPLEX_STATUS_DOMAIN);
        return value(0.0, 0.0);
    }
    return exponential(multiply(exponent, logarithm(parser, base)));
}

static bool identifier_equals(const char *identifier, size_t length,
                              const char *expected) {
    if (strlen(expected) != length) return false;
    for (size_t i = 0; i < length; ++i) {
        if (tolower((unsigned char)identifier[i]) != expected[i]) return false;
    }
    return true;
}

static bool nearly_real(complex_value_t input) {
    return fabs(input.imag) <= 1e-12 * fmax(1.0, fabs(input.real));
}

static complex_value_t apply_function(parser_t *parser,
                                      const char *name, size_t length,
                                      complex_value_t first,
                                      bool has_second,
                                      complex_value_t second) {
    if (identifier_equals(name, length, "conj") && !has_second) {
        return value(first.real, -first.imag);
    }
    if (identifier_equals(name, length, "abs") && !has_second) {
        return value(hypot(first.real, first.imag), 0.0);
    }
    if (identifier_equals(name, length, "arg") && !has_second) {
        if (first.real == 0.0 && first.imag == 0.0) {
            set_error(parser, COMPLEX_STATUS_DOMAIN);
            return value(0.0, 0.0);
        }
        double angle = atan2(first.imag, first.real);
        if (parser->degrees) angle *= 180.0 / PI_VALUE;
        return value(angle, 0.0);
    }
    if (identifier_equals(name, length, "re") && !has_second) {
        return value(first.real, 0.0);
    }
    if (identifier_equals(name, length, "im") && !has_second) {
        return value(first.imag, 0.0);
    }
    if (identifier_equals(name, length, "exp") && !has_second) {
        return exponential(first);
    }
    if (identifier_equals(name, length, "ln") && !has_second) {
        return logarithm(parser, first);
    }
    if (identifier_equals(name, length, "sqrt") && !has_second) {
        return power(parser, first, value(0.5, 0.0));
    }
    if (identifier_equals(name, length, "polar") && has_second) {
        if (!nearly_real(first) || !nearly_real(second) || first.real < 0.0) {
            set_error(parser, COMPLEX_STATUS_DOMAIN);
            return value(0.0, 0.0);
        }
        double angle = second.real;
        if (parser->degrees) angle *= PI_VALUE / 180.0;
        return value(first.real * cos(angle), first.real * sin(angle));
    }
    set_error(parser, COMPLEX_STATUS_SYNTAX);
    return value(0.0, 0.0);
}

static complex_value_t parse_primary(parser_t *parser) {
    skip_spaces(parser);
    if (*parser->cursor == '(') {
        parser->cursor++;
        complex_value_t result = parse_expression(parser);
        skip_spaces(parser);
        if (*parser->cursor != ')') {
            set_error(parser, COMPLEX_STATUS_SYNTAX);
            return value(0.0, 0.0);
        }
        parser->cursor++;
        return result;
    }
    if (isdigit((unsigned char)*parser->cursor) || *parser->cursor == '.') {
        char *end = NULL;
        double number = strtod(parser->cursor, &end);
        if (end == parser->cursor) {
            set_error(parser, COMPLEX_STATUS_SYNTAX);
            return value(0.0, 0.0);
        }
        parser->cursor = end;
        /* Only overflow is a range error; ERANGE underflow still yields a
         * usable subnormal value. */
        if (!isfinite(number)) {
            set_error(parser, COMPLEX_STATUS_RANGE);
        }
        return value(number, 0.0);
    }
    if (!isalpha((unsigned char)*parser->cursor)) {
        set_error(parser, COMPLEX_STATUS_SYNTAX);
        return value(0.0, 0.0);
    }

    const char *name = parser->cursor;
    while (isalpha((unsigned char)*parser->cursor)) parser->cursor++;
    size_t length = (size_t)(parser->cursor - name);
    if (identifier_equals(name, length, "i")) return value(0.0, 1.0);
    if (identifier_equals(name, length, "pi")) return value(PI_VALUE, 0.0);
    if (identifier_equals(name, length, "e")) return value(exp(1.0), 0.0);

    skip_spaces(parser);
    if (*parser->cursor != '(') {
        set_error(parser, COMPLEX_STATUS_SYNTAX);
        return value(0.0, 0.0);
    }
    parser->cursor++;
    complex_value_t first = parse_expression(parser);
    skip_spaces(parser);
    bool has_second = false;
    complex_value_t second = value(0.0, 0.0);
    if (*parser->cursor == ',') {
        has_second = true;
        parser->cursor++;
        second = parse_expression(parser);
        skip_spaces(parser);
    }
    if (*parser->cursor != ')') {
        set_error(parser, COMPLEX_STATUS_SYNTAX);
        return value(0.0, 0.0);
    }
    parser->cursor++;
    return apply_function(parser, name, length, first, has_second, second);
}

static complex_value_t parse_unary(parser_t *parser);

static complex_value_t parse_power(parser_t *parser) {
    complex_value_t left = parse_primary(parser);
    skip_spaces(parser);
    if (parser->status == COMPLEX_STATUS_OK && *parser->cursor == '^') {
        parser->cursor++;
        left = power(parser, left, parse_unary(parser));
    }
    return left;
}

static complex_value_t parse_unary(parser_t *parser) {
    skip_spaces(parser);
    if (*parser->cursor == '+' || *parser->cursor == '-') {
        bool negative = *parser->cursor == '-';
        parser->cursor++;
        complex_value_t result = parse_unary(parser);
        return negative ? value(-result.real, -result.imag) : result;
    }
    return parse_power(parser);
}

static bool starts_primary(char current) {
    return current == '(' || current == '.' ||
           isdigit((unsigned char)current) || isalpha((unsigned char)current);
}

static complex_value_t parse_term(parser_t *parser) {
    complex_value_t left = parse_unary(parser);
    while (parser->status == COMPLEX_STATUS_OK) {
        skip_spaces(parser);
        char operation = *parser->cursor;
        bool implicit = starts_primary(operation);
        if (operation != '*' && operation != '/' && !implicit) break;
        if (!implicit) parser->cursor++;
        complex_value_t right = parse_unary(parser);
        left = operation == '/' ? divide(parser, left, right)
                                : multiply(left, right);
        if (!finite_value(left)) set_error(parser, COMPLEX_STATUS_RANGE);
    }
    return left;
}

static complex_value_t parse_expression(parser_t *parser) {
    complex_value_t left = parse_term(parser);
    while (parser->status == COMPLEX_STATUS_OK) {
        skip_spaces(parser);
        char operation = *parser->cursor;
        if (operation != '+' && operation != '-') break;
        parser->cursor++;
        complex_value_t right = parse_term(parser);
        left = operation == '+' ? add(left, right) : subtract(left, right);
        if (!finite_value(left)) set_error(parser, COMPLEX_STATUS_RANGE);
    }
    return left;
}

complex_status_t complex_engine_evaluate(const char *expression,
                                         bool degrees,
                                         complex_value_t *result,
                                         int *error_position) {
    if (error_position) *error_position = 0;
    if (!expression || !result) return COMPLEX_STATUS_EMPTY;
    parser_t parser = {
        .expression = expression,
        .cursor = expression,
        .degrees = degrees,
        .status = COMPLEX_STATUS_OK,
    };
    skip_spaces(&parser);
    if (!*parser.cursor) return COMPLEX_STATUS_EMPTY;
    complex_value_t evaluated = parse_expression(&parser);
    skip_spaces(&parser);
    if (parser.status == COMPLEX_STATUS_OK && *parser.cursor) {
        set_error(&parser, COMPLEX_STATUS_SYNTAX);
    }
    if (parser.status == COMPLEX_STATUS_OK && !finite_value(evaluated)) {
        set_error(&parser, COMPLEX_STATUS_RANGE);
    }
    if (error_position) *error_position = parser.error_position;
    if (parser.status == COMPLEX_STATUS_OK) *result = evaluated;
    return parser.status;
}

bool complex_engine_format(complex_value_t input, bool polar, bool degrees,
                           char *output, size_t output_size) {
    if (!output || !output_size || !finite_value(input)) return false;
    int written;
    if (polar) {
        double magnitude = hypot(input.real, input.imag);
        double angle = atan2(input.imag, input.real);
        if (degrees) angle *= 180.0 / PI_VALUE;
        written = snprintf(output, output_size, "%.10g < %.10g %s",
                           magnitude, angle, degrees ? "deg" : "rad");
    } else {
        double real = input.real == 0.0 ? 0.0 : input.real;
        double imag = input.imag == 0.0 ? 0.0 : input.imag;
        written = snprintf(output, output_size, "%.10g%+.10gi", real, imag);
    }
    return written >= 0 && (size_t)written < output_size;
}

const char *complex_engine_status_text(complex_status_t status) {
    switch (status) {
        case COMPLEX_STATUS_OK: return "OK";
        case COMPLEX_STATUS_EMPTY: return "EMPTY";
        case COMPLEX_STATUS_SYNTAX: return "SYNTAX";
        case COMPLEX_STATUS_DIV_ZERO: return "DIVISION BY ZERO";
        case COMPLEX_STATUS_DOMAIN: return "DOMAIN ERROR";
        case COMPLEX_STATUS_RANGE: return "NUMBER RANGE";
        default: return "COMPLEX ERROR";
    }
}
