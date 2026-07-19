#include "high_precision_engine.h"

#include "libbf.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HP_MAX_DEPTH 12u
#define HP_FLAGS (BF_RNDN | BF_FLAG_EXT_EXP)

typedef struct {
    bf_context_t *context;
    const char *start;
    const char *cursor;
    const char *ans_text;
    const calculator_symbols_t *symbols;
    const bf_t *x;
    bool degrees;
    limb_t work_bits;
    unsigned int decimal_digits;
    unsigned int active_functions;
    unsigned int depth;
    int bf_status;
    high_precision_status_t status;
    int error_position;
} hp_parser_t;

static void *hp_realloc(void *opaque, void *pointer, size_t size) {
    (void)opaque;
    if (size == 0u) {
        free(pointer);
        return NULL;
    }
    return realloc(pointer, size);
}

static void hp_skip_space(hp_parser_t *parser) {
    while (isspace((unsigned char)*parser->cursor)) parser->cursor++;
}

static void hp_fail_at(hp_parser_t *parser, high_precision_status_t status,
                       const char *position) {
    if (parser->status != HIGH_PRECISION_STATUS_OK) return;
    parser->status = status;
    parser->error_position = (int)(position - parser->start) + 1;
}

static void hp_fail(hp_parser_t *parser, high_precision_status_t status) {
    hp_fail_at(parser, status, parser->cursor);
}

static void hp_move(bf_t *destination, bf_t *source) {
    bf_move(destination, source);
    source->ctx = NULL;
    source->tab = NULL;
}

static bool hp_accept_status(hp_parser_t *parser, int status,
                             const bf_t *value, const char *position) {
    parser->bf_status |= status;
    if (status & BF_ST_MEM_ERROR) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_MEMORY, position);
    } else if (status & BF_ST_DIVIDE_ZERO) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_DIV_ZERO, position);
    } else if ((status & BF_ST_INVALID_OP) || bf_is_nan(value)) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_DOMAIN, position);
    } else if ((status & (BF_ST_OVERFLOW | BF_ST_UNDERFLOW)) ||
               !bf_is_finite(value)) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_RANGE, position);
    }
    return parser->status == HIGH_PRECISION_STATUS_OK;
}

static bool identifier_is(const char *name, size_t length,
                          const char *expected) {
    size_t expected_length = strlen(expected);
    if (length != expected_length) return false;
    for (size_t i = 0; i < length; ++i) {
        if (tolower((unsigned char)name[i]) !=
            tolower((unsigned char)expected[i])) {
            return false;
        }
    }
    return true;
}

static bool hp_set_text(hp_parser_t *parser, bf_t *value, const char *text,
                        const char *position) {
    const char *end = NULL;
    int status = bf_atof(value, text, &end, 10, parser->work_bits,
                         HP_FLAGS | BF_ATOF_NO_NAN_INF);
    if (!end || end == text || *end) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_SYNTAX, position);
        return false;
    }
    return hp_accept_status(parser, status, value, position);
}

static bool hp_set_double(hp_parser_t *parser, bf_t *value, double input,
                          const char *position) {
    if (!isfinite(input)) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_RANGE, position);
        return false;
    }
    int status = bf_set_float64(value, input);
    return hp_accept_status(parser, status, value, position);
}

static bool hp_constant_pi(hp_parser_t *parser, bf_t *result,
                           const char *position) {
    int status = bf_const_pi(result, parser->work_bits, HP_FLAGS);
    return hp_accept_status(parser, status, result, position);
}

static bool hp_constant_e(hp_parser_t *parser, bf_t *result,
                          const char *position) {
    bf_t one;
    bf_init(parser->context, &one);
    int status = bf_set_ui(&one, 1u);
    if (hp_accept_status(parser, status, &one, position)) {
        status = bf_exp(result, &one, parser->work_bits, HP_FLAGS);
        hp_accept_status(parser, status, result, position);
    }
    bf_delete(&one);
    return parser->status == HIGH_PRECISION_STATUS_OK;
}

static bool hp_constant_phi(hp_parser_t *parser, bf_t *result,
                            const char *position) {
    bf_t five;
    bf_t root;
    bf_init(parser->context, &five);
    bf_init(parser->context, &root);
    int status = bf_set_ui(&five, 5u);
    if (hp_accept_status(parser, status, &five, position)) {
        status = bf_sqrt(&root, &five, parser->work_bits, HP_FLAGS);
    }
    if (parser->status == HIGH_PRECISION_STATUS_OK &&
        hp_accept_status(parser, status, &root, position)) {
        status = bf_add_si(&root, &root, 1, parser->work_bits, HP_FLAGS);
    }
    if (parser->status == HIGH_PRECISION_STATUS_OK &&
        hp_accept_status(parser, status, &root, position)) {
        status = bf_mul_2exp(&root, -1, parser->work_bits, HP_FLAGS);
    }
    if (parser->status == HIGH_PRECISION_STATUS_OK &&
        hp_accept_status(parser, status, &root, position)) {
        hp_move(result, &root);
    }
    bf_delete(&five);
    bf_delete(&root);
    return parser->status == HIGH_PRECISION_STATUS_OK;
}

static bool hp_apply_binary(hp_parser_t *parser, char operation,
                            bf_t *left, const bf_t *right,
                            const char *position) {
    bf_t combined;
    bf_init(parser->context, &combined);
    int status = 0;
    switch (operation) {
        case '+':
            status = bf_add(&combined, left, right, parser->work_bits, HP_FLAGS);
            break;
        case '-':
            status = bf_sub(&combined, left, right, parser->work_bits, HP_FLAGS);
            break;
        case '*':
            status = bf_mul(&combined, left, right, parser->work_bits, HP_FLAGS);
            break;
        case '/':
            status = bf_div(&combined, left, right, parser->work_bits, HP_FLAGS);
            break;
        case '%':
            status = bf_rem(&combined, left, right, parser->work_bits, HP_FLAGS,
                            BF_RNDZ);
            break;
        case '^':
            status = bf_pow(&combined, left, right, parser->work_bits, HP_FLAGS);
            break;
        default:
            hp_fail_at(parser, HIGH_PRECISION_STATUS_SYNTAX, position);
            break;
    }
    if (parser->status == HIGH_PRECISION_STATUS_OK &&
        hp_accept_status(parser, status, &combined, position)) {
        hp_move(left, &combined);
    }
    bf_delete(&combined);
    return parser->status == HIGH_PRECISION_STATUS_OK;
}

static bool hp_scale_angle(hp_parser_t *parser, bf_t *value, bool to_radians,
                           const char *position) {
    if (!parser->degrees) return true;
    bf_t pi;
    bf_t divisor;
    bf_t converted;
    bf_init(parser->context, &pi);
    bf_init(parser->context, &divisor);
    bf_init(parser->context, &converted);
    int status = bf_set_ui(&divisor, 180u);
    if (hp_accept_status(parser, status, &divisor, position) &&
        hp_constant_pi(parser, &pi, position)) {
        if (to_radians) {
            status = bf_mul(&converted, value, &pi, parser->work_bits, HP_FLAGS);
            if (hp_accept_status(parser, status, &converted, position)) {
                status = bf_div(&converted, &converted, &divisor,
                                parser->work_bits, HP_FLAGS);
            }
        } else {
            status = bf_mul(&converted, value, &divisor,
                            parser->work_bits, HP_FLAGS);
            if (hp_accept_status(parser, status, &converted, position)) {
                status = bf_div(&converted, &converted, &pi,
                                parser->work_bits, HP_FLAGS);
            }
        }
        if (parser->status == HIGH_PRECISION_STATUS_OK &&
            hp_accept_status(parser, status, &converted, position)) {
            hp_move(value, &converted);
        }
    }
    bf_delete(&pi);
    bf_delete(&divisor);
    bf_delete(&converted);
    return parser->status == HIGH_PRECISION_STATUS_OK;
}

static bool hp_small_integer(const bf_t *value, int maximum, int *integer) {
    bf_t rounded;
    bf_init(value->ctx, &rounded);
    if (bf_set(&rounded, value) != 0) {
        bf_delete(&rounded);
        return false;
    }
    (void)bf_rint(&rounded, BF_RNDZ);
    bool valid = bf_cmp(&rounded, value) == 0 &&
                 bf_get_int32(integer, &rounded, 0) == 0 &&
                 *integer >= 0 && *integer <= maximum;
    bf_delete(&rounded);
    return valid;
}

static bool hp_factorial(hp_parser_t *parser, bf_t *result,
                         const bf_t *argument, const char *position) {
    int n = 0;
    if (!hp_small_integer(argument, 170, &n)) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_DOMAIN, position);
        return false;
    }
    int status = bf_set_ui(result, 1u);
    for (int i = 2; i <= n &&
         hp_accept_status(parser, status, result, position); ++i) {
        status = bf_mul_ui(result, result, (uint64_t)i,
                           parser->work_bits, HP_FLAGS);
    }
    return hp_accept_status(parser, status, result, position);
}

static bool hp_combinatoric(hp_parser_t *parser, bf_t *result,
                            const bf_t *first, const bf_t *second,
                            bool permutation, const char *position) {
    int n = 0;
    int r = 0;
    if (!hp_small_integer(first, 170, &n) ||
        !hp_small_integer(second, 170, &r) || r > n) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_DOMAIN, position);
        return false;
    }
    if (!permutation && r > n - r) r = n - r;
    int status = bf_set_ui(result, 1u);
    for (int i = 1; i <= r &&
         hp_accept_status(parser, status, result, position); ++i) {
        int factor = permutation ? n - i + 1 : n - r + i;
        status = bf_mul_ui(result, result, (uint64_t)factor,
                           parser->work_bits, HP_FLAGS);
        if (!permutation &&
            hp_accept_status(parser, status, result, position)) {
            bf_t denominator;
            bf_t quotient;
            bf_init(parser->context, &denominator);
            bf_init(parser->context, &quotient);
            status = bf_set_ui(&denominator, (uint64_t)i);
            if (hp_accept_status(parser, status, &denominator, position)) {
                status = bf_div(&quotient, result, &denominator,
                                parser->work_bits, HP_FLAGS);
            }
            if (hp_accept_status(parser, status, &quotient, position)) {
                hp_move(result, &quotient);
            }
            bf_delete(&denominator);
            bf_delete(&quotient);
        }
    }
    return parser->status == HIGH_PRECISION_STATUS_OK;
}

static bool hp_hyperbolic(hp_parser_t *parser, bf_t *result,
                          const bf_t *argument, const char *name,
                          size_t name_length, const char *position) {
    bf_t positive;
    bf_t negative_argument;
    bf_t negative;
    bf_t combined;
    bf_init(parser->context, &positive);
    bf_init(parser->context, &negative_argument);
    bf_init(parser->context, &negative);
    bf_init(parser->context, &combined);
    int status = bf_exp(&positive, argument, parser->work_bits, HP_FLAGS);
    if (hp_accept_status(parser, status, &positive, position)) {
        status = bf_set(&negative_argument, argument);
    }
    if (hp_accept_status(parser, status, &negative_argument, position)) {
        bf_neg(&negative_argument);
        status = bf_exp(&negative, &negative_argument,
                        parser->work_bits, HP_FLAGS);
    }
    if (hp_accept_status(parser, status, &negative, position)) {
        bool cosine = identifier_is(name, name_length, "cosh");
        status = cosine
            ? bf_add(&combined, &positive, &negative,
                     parser->work_bits, HP_FLAGS)
            : bf_sub(&combined, &positive, &negative,
                     parser->work_bits, HP_FLAGS);
    }
    if (hp_accept_status(parser, status, &combined, position)) {
        if (identifier_is(name, name_length, "tanh")) {
            bf_t denominator;
            bf_t quotient;
            bf_init(parser->context, &denominator);
            bf_init(parser->context, &quotient);
            status = bf_add(&denominator, &positive, &negative,
                            parser->work_bits, HP_FLAGS);
            if (hp_accept_status(parser, status, &denominator, position)) {
                status = bf_div(&quotient, &combined, &denominator,
                                parser->work_bits, HP_FLAGS);
            }
            if (hp_accept_status(parser, status, &quotient, position)) {
                hp_move(result, &quotient);
            }
            bf_delete(&denominator);
            bf_delete(&quotient);
        } else {
            status = bf_mul_2exp(&combined, -1,
                                 parser->work_bits, HP_FLAGS);
            if (hp_accept_status(parser, status, &combined, position)) {
                hp_move(result, &combined);
            }
        }
    }
    bf_delete(&positive);
    bf_delete(&negative_argument);
    bf_delete(&negative);
    bf_delete(&combined);
    return parser->status == HIGH_PRECISION_STATUS_OK;
}

static bool hp_apply_function(hp_parser_t *parser, const char *name,
                              size_t name_length, bf_t *result,
                              bf_t *first, bf_t *second, bool has_second,
                              const char *position) {
    int status = 0;
    if (identifier_is(name, name_length, "abs") && !has_second) {
        status = bf_set(result, first);
        result->sign = 0;
    } else if (identifier_is(name, name_length, "floor") && !has_second) {
        status = bf_set(result, first);
        if (status == 0) status = bf_rint(result, BF_RNDD);
    } else if (identifier_is(name, name_length, "ceil") && !has_second) {
        status = bf_set(result, first);
        if (status == 0) status = bf_rint(result, BF_RNDU);
    } else if (identifier_is(name, name_length, "sqrt") && !has_second) {
        status = bf_sqrt(result, first, parser->work_bits, HP_FLAGS);
    } else if (identifier_is(name, name_length, "exp") && !has_second) {
        status = bf_exp(result, first, parser->work_bits, HP_FLAGS);
    } else if (identifier_is(name, name_length, "ln") && !has_second) {
        status = bf_log(result, first, parser->work_bits, HP_FLAGS);
    } else if ((identifier_is(name, name_length, "log") ||
                identifier_is(name, name_length, "log10")) && !has_second) {
        bf_t ten;
        bf_t denominator;
        bf_init(parser->context, &ten);
        bf_init(parser->context, &denominator);
        status = bf_log(result, first, parser->work_bits, HP_FLAGS);
        if (hp_accept_status(parser, status, result, position)) {
            status = bf_set_ui(&ten, 10u);
        }
        if (hp_accept_status(parser, status, &ten, position)) {
            status = bf_log(&denominator, &ten,
                            parser->work_bits, HP_FLAGS);
        }
        if (hp_accept_status(parser, status, &denominator, position)) {
            status = bf_div(result, result, &denominator,
                            parser->work_bits, HP_FLAGS);
        }
        bf_delete(&ten);
        bf_delete(&denominator);
    } else if ((identifier_is(name, name_length, "sin") ||
                identifier_is(name, name_length, "cos") ||
                identifier_is(name, name_length, "tan")) && !has_second) {
        if (!hp_scale_angle(parser, first, true, position)) return false;
        if (identifier_is(name, name_length, "sin")) {
            status = bf_sin(result, first, parser->work_bits, HP_FLAGS);
        } else if (identifier_is(name, name_length, "cos")) {
            status = bf_cos(result, first, parser->work_bits, HP_FLAGS);
        } else {
            status = bf_tan(result, first, parser->work_bits, HP_FLAGS);
        }
    } else if ((identifier_is(name, name_length, "asin") ||
                identifier_is(name, name_length, "acos") ||
                identifier_is(name, name_length, "atan")) && !has_second) {
        if (identifier_is(name, name_length, "asin")) {
            status = bf_asin(result, first, parser->work_bits, HP_FLAGS);
        } else if (identifier_is(name, name_length, "acos")) {
            status = bf_acos(result, first, parser->work_bits, HP_FLAGS);
        } else {
            status = bf_atan(result, first, parser->work_bits, HP_FLAGS);
        }
        if (hp_accept_status(parser, status, result, position) &&
            !hp_scale_angle(parser, result, false, position)) {
            return false;
        }
    } else if ((identifier_is(name, name_length, "sinh") ||
                identifier_is(name, name_length, "cosh") ||
                identifier_is(name, name_length, "tanh")) && !has_second) {
        return hp_hyperbolic(parser, result, first, name, name_length,
                             position);
    } else if (identifier_is(name, name_length, "fac") && !has_second) {
        return hp_factorial(parser, result, first, position);
    } else if (identifier_is(name, name_length, "pow") && has_second) {
        status = bf_pow(result, first, second,
                        parser->work_bits, HP_FLAGS);
    } else if (identifier_is(name, name_length, "atan2") && has_second) {
        status = bf_atan2(result, first, second,
                          parser->work_bits, HP_FLAGS);
        if (hp_accept_status(parser, status, result, position) &&
            !hp_scale_angle(parser, result, false, position)) {
            return false;
        }
    } else if (identifier_is(name, name_length, "ncr") && has_second) {
        return hp_combinatoric(parser, result, first, second, false, position);
    } else if (identifier_is(name, name_length, "npr") && has_second) {
        return hp_combinatoric(parser, result, first, second, true, position);
    } else {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_UNSUPPORTED, position);
        return false;
    }
    return hp_accept_status(parser, status, result, position);
}

static bool hp_parse_expression(hp_parser_t *parser, bf_t *value);
static bool hp_parse_unary(hp_parser_t *parser, bf_t *value);

static bool hp_evaluate_user_function(hp_parser_t *parser, size_t function,
                                      const bf_t *argument, bf_t *result,
                                      const char *position) {
    if (!parser->symbols ||
        !parser->symbols->functions[function][0] ||
        (parser->active_functions & (1u << function))) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_UNSUPPORTED, position);
        return false;
    }
    hp_parser_t child = {
        .context = parser->context,
        .start = parser->symbols->functions[function],
        .cursor = parser->symbols->functions[function],
        .ans_text = parser->ans_text,
        .symbols = parser->symbols,
        .x = argument,
        .degrees = parser->degrees,
        .work_bits = parser->work_bits,
        .decimal_digits = parser->decimal_digits,
        .active_functions = parser->active_functions | (1u << function),
        .depth = parser->depth + 1u,
        .status = HIGH_PRECISION_STATUS_OK,
    };
    if (child.depth > HP_MAX_DEPTH ||
        !hp_parse_expression(&child, result)) {
        hp_fail_at(parser, child.status == HIGH_PRECISION_STATUS_OK
            ? HIGH_PRECISION_STATUS_RANGE : child.status, position);
        return false;
    }
    hp_skip_space(&child);
    if (*child.cursor) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_SYNTAX, position);
        return false;
    }
    parser->bf_status |= child.bf_status;
    return true;
}

static bool hp_parse_identifier(hp_parser_t *parser, bf_t *value) {
    const char *position = parser->cursor;
    const char *name = parser->cursor++;
    while (isalnum((unsigned char)*parser->cursor) ||
           *parser->cursor == '_') {
        parser->cursor++;
    }
    size_t length = (size_t)(parser->cursor - name);
    hp_skip_space(parser);

    if (*parser->cursor != '(') {
        if (identifier_is(name, length, "ans")) {
            return hp_set_text(parser, value,
                               parser->ans_text && *parser->ans_text
                                   ? parser->ans_text : "0",
                               position);
        }
        if (length == 1u && name[0] >= 'A' && name[0] <= 'F' &&
            parser->symbols) {
            size_t index = (size_t)(name[0] - 'A');
            const char *text = parser->symbols->variable_text[index];
            if (text[0]) return hp_set_text(parser, value, text, position);
            return hp_set_double(parser, value, parser->symbols->variables[index],
                                 position);
        }
        if (length == 1u && name[0] == 'x' && parser->x) {
            int status = bf_set(value, parser->x);
            return hp_accept_status(parser, status, value, position);
        }
        if (identifier_is(name, length, "pi")) {
            return hp_constant_pi(parser, value, position);
        }
        if (identifier_is(name, length, "e")) {
            return hp_constant_e(parser, value, position);
        }
        if (identifier_is(name, length, "tau")) {
            if (!hp_constant_pi(parser, value, position)) return false;
            int status = bf_mul_ui(value, value, 2u,
                                   parser->work_bits, HP_FLAGS);
            return hp_accept_status(parser, status, value, position);
        }
        if (identifier_is(name, length, "phi")) {
            return hp_constant_phi(parser, value, position);
        }
        hp_fail_at(parser, HIGH_PRECISION_STATUS_UNSUPPORTED, position);
        return false;
    }

    parser->cursor++;
    bf_t first;
    bf_t second;
    bf_t function_result;
    bf_init(parser->context, &first);
    bf_init(parser->context, &second);
    bf_init(parser->context, &function_result);
    bool has_second = false;
    if (++parser->depth > HP_MAX_DEPTH ||
        !hp_parse_expression(parser, &first)) {
        if (parser->status == HIGH_PRECISION_STATUS_OK) {
            hp_fail_at(parser, HIGH_PRECISION_STATUS_RANGE, position);
        }
        goto done;
    }
    hp_skip_space(parser);
    if (*parser->cursor == ',') {
        has_second = true;
        parser->cursor++;
        if (!hp_parse_expression(parser, &second)) goto done;
        hp_skip_space(parser);
    }
    if (*parser->cursor != ')') {
        hp_fail(parser, HIGH_PRECISION_STATUS_SYNTAX);
        goto done;
    }
    parser->cursor++;
    parser->depth--;

    if (length == 2u && tolower((unsigned char)name[0]) == 'f' &&
        name[1] >= '1' && name[1] <= '3' && !has_second) {
        hp_evaluate_user_function(parser, (size_t)(name[1] - '1'), &first,
                                  &function_result, position);
    } else {
        hp_apply_function(parser, name, length, &function_result,
                          &first, &second, has_second, position);
    }
    if (parser->status == HIGH_PRECISION_STATUS_OK) {
        hp_move(value, &function_result);
    }

done:
    bf_delete(&first);
    bf_delete(&second);
    bf_delete(&function_result);
    return parser->status == HIGH_PRECISION_STATUS_OK;
}

static bool hp_parse_primary(hp_parser_t *parser, bf_t *value) {
    hp_skip_space(parser);
    if (*parser->cursor == '(') {
        const char *position = parser->cursor++;
        if (++parser->depth > HP_MAX_DEPTH ||
            !hp_parse_expression(parser, value)) {
            if (parser->status == HIGH_PRECISION_STATUS_OK) {
                hp_fail_at(parser, HIGH_PRECISION_STATUS_RANGE, position);
            }
            return false;
        }
        hp_skip_space(parser);
        if (*parser->cursor != ')') {
            hp_fail(parser, HIGH_PRECISION_STATUS_SYNTAX);
            return false;
        }
        parser->cursor++;
        parser->depth--;
        return true;
    }
    if (isalpha((unsigned char)*parser->cursor)) {
        return hp_parse_identifier(parser, value);
    }
    if (!isdigit((unsigned char)*parser->cursor) &&
        !(*parser->cursor == '.' &&
          isdigit((unsigned char)parser->cursor[1]))) {
        hp_fail(parser, HIGH_PRECISION_STATUS_SYNTAX);
        return false;
    }
    const char *position = parser->cursor;
    const char *end = NULL;
    int status = bf_atof(value, parser->cursor, &end, 10,
                         parser->work_bits,
                         HP_FLAGS | BF_ATOF_NO_NAN_INF);
    if (!end || end == parser->cursor) {
        hp_fail_at(parser, HIGH_PRECISION_STATUS_SYNTAX, position);
        return false;
    }
    parser->cursor = end;
    return hp_accept_status(parser, status, value, position);
}

static bool hp_parse_power(hp_parser_t *parser, bf_t *value) {
    if (!hp_parse_primary(parser, value)) return false;
    hp_skip_space(parser);
    if (*parser->cursor == '^') {
        const char *position = parser->cursor++;
        bf_t exponent;
        bf_init(parser->context, &exponent);
        if (hp_parse_unary(parser, &exponent)) {
            hp_apply_binary(parser, '^', value, &exponent, position);
        }
        bf_delete(&exponent);
    }
    return parser->status == HIGH_PRECISION_STATUS_OK;
}

static bool hp_parse_unary(hp_parser_t *parser, bf_t *value) {
    hp_skip_space(parser);
    if (*parser->cursor == '+' || *parser->cursor == '-') {
        bool negative = *parser->cursor++ == '-';
        if (!hp_parse_unary(parser, value)) return false;
        if (negative && !bf_is_zero(value)) bf_neg(value);
        return true;
    }
    return hp_parse_power(parser, value);
}

static bool hp_parse_term(hp_parser_t *parser, bf_t *value) {
    if (!hp_parse_unary(parser, value)) return false;
    for (;;) {
        hp_skip_space(parser);
        char operation = *parser->cursor;
        if (operation != '*' && operation != '/' && operation != '%') {
            return true;
        }
        const char *position = parser->cursor++;
        bf_t right;
        bf_init(parser->context, &right);
        if (hp_parse_unary(parser, &right)) {
            hp_apply_binary(parser, operation, value, &right, position);
        }
        bf_delete(&right);
        if (parser->status != HIGH_PRECISION_STATUS_OK) return false;
    }
}

static bool hp_parse_expression(hp_parser_t *parser, bf_t *value) {
    if (!hp_parse_term(parser, value)) return false;
    for (;;) {
        hp_skip_space(parser);
        char operation = *parser->cursor;
        if (operation != '+' && operation != '-') return true;
        const char *position = parser->cursor++;
        bf_t right;
        bf_init(parser->context, &right);
        if (hp_parse_term(parser, &right)) {
            hp_apply_binary(parser, operation, value, &right, position);
        }
        bf_delete(&right);
        if (parser->status != HIGH_PRECISION_STATUS_OK) return false;
    }
}

static void trim_formatted_number(char *text) {
    char *exponent = strchr(text, 'e');
    if (!exponent) exponent = strchr(text, 'E');
    char *end = exponent ? exponent : text + strlen(text);
    char *point = strchr(text, '.');
    if (!point || point >= end) return;
    char *trim = end;
    while (trim > point + 1 && trim[-1] == '0') trim--;
    if (trim == point + 1) trim = point;
    if (exponent) {
        memmove(trim, exponent, strlen(exponent) + 1u);
    } else {
        *trim = '\0';
    }
}

high_precision_status_t high_precision_engine_evaluate(
    const char *expression, const char *ans_text,
    const calculator_symbols_t *symbols, bool degrees,
    calculator_precision_t precision,
    high_precision_result_t *result, int *error_position) {
    if (error_position) *error_position = 0;
    if (!expression || !*expression || !result ||
        precision >= CALCULATOR_PRECISION_COUNT) {
        return HIGH_PRECISION_STATUS_SYNTAX;
    }

    bf_context_t context;
    bf_context_init(&context, hp_realloc, NULL);
    hp_parser_t parser = {
        .context = &context,
        .start = expression,
        .cursor = expression,
        .ans_text = ans_text,
        .symbols = symbols,
        .degrees = degrees,
        .work_bits = calculator_precision_work_bits(precision),
        .decimal_digits = calculator_precision_digits(precision),
        .status = HIGH_PRECISION_STATUS_OK,
    };
    bf_t value;
    bf_init(&context, &value);
    if (hp_parse_expression(&parser, &value)) {
        hp_skip_space(&parser);
        if (*parser.cursor) hp_fail(&parser, HIGH_PRECISION_STATUS_SYNTAX);
    }

    if (parser.status == HIGH_PRECISION_STATUS_OK) {
        size_t length = 0u;
        char *formatted = bf_ftoa(
            &length, &value, 10, parser.decimal_digits,
            BF_RNDN | BF_FTOA_FORMAT_FIXED);
        if (!formatted) {
            hp_fail(&parser, HIGH_PRECISION_STATUS_MEMORY);
        } else if (length >= sizeof result->text) {
            hp_fail(&parser, HIGH_PRECISION_STATUS_RANGE);
        } else {
            memcpy(result->text, formatted, length + 1u);
            trim_formatted_number(result->text);
            int status = bf_get_float64(&value, &result->approximation,
                                        BF_RNDN);
            if ((status & BF_ST_INVALID_OP) ||
                !isfinite(result->approximation)) {
                hp_fail(&parser, HIGH_PRECISION_STATUS_RANGE);
            }
        }
        bf_free(&context, formatted);
    }

    if (error_position && parser.status != HIGH_PRECISION_STATUS_OK) {
        *error_position = parser.error_position;
    }
    bf_delete(&value);
    bf_context_end(&context);
    return parser.status;
}

const char *high_precision_status_text(high_precision_status_t status) {
    switch (status) {
        case HIGH_PRECISION_STATUS_OK: return "OK";
        case HIGH_PRECISION_STATUS_UNSUPPORTED: return "UNSUPPORTED";
        case HIGH_PRECISION_STATUS_SYNTAX: return "SYNTAX";
        case HIGH_PRECISION_STATUS_DIV_ZERO: return "DIV ZERO";
        case HIGH_PRECISION_STATUS_DOMAIN: return "DOMAIN";
        case HIGH_PRECISION_STATUS_RANGE: return "RANGE";
        case HIGH_PRECISION_STATUS_MEMORY: return "MEMORY";
        default: return "ERROR";
    }
}
