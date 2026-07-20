#include "calculator_number_theory.h"

#include "number_theory.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static void set_message(char *message, size_t size, const char *text) {
    if (message && size) snprintf(message, size, "%s", text);
}

static bool parse_uint64(const char *text, uint64_t *value) {
    if (!text || !*text || !value) return false;
    uint64_t parsed = 0;
    for (size_t i = 0; text[i]; ++i) {
        if (text[i] < '0' || text[i] > '9') return false;
        unsigned int digit = (unsigned int)(text[i] - '0');
        if (parsed > (UINT64_MAX - digit) / 10u) return false;
        parsed = parsed * 10u + digit;
    }
    *value = parsed;
    return true;
}

static void format_uint64(char *text, size_t size, uint64_t value) {
    snprintf(text, size, "%llu", (unsigned long long)value);
}

static void set_result(calculator_number_theory_t *tool,
                       const char *operation, uint64_t value) {
    tool->last_result = value;
    tool->has_result = true;
    snprintf(tool->operation, sizeof tool->operation, "%s", operation);
    tool->result[0] = '=';
    format_uint64(tool->result + 1u, sizeof tool->result - 1u, value);
}

void calculator_number_theory_init(calculator_number_theory_t *tool) {
    memset(tool, 0, sizeof *tool);
    snprintf(tool->inputs[0], sizeof tool->inputs[0], "0");
    snprintf(tool->inputs[1], sizeof tool->inputs[1], "0");
    snprintf(tool->inputs[2], sizeof tool->inputs[2], "1");
    snprintf(tool->operation, sizeof tool->operation, "READY");
    snprintf(tool->result, sizeof tool->result, "Select A, B or M");
}

static bool read_inputs(const calculator_number_theory_t *tool,
                        uint64_t *a, uint64_t *b, uint64_t *modulus) {
    return parse_uint64(tool->inputs[0], a) &&
           parse_uint64(tool->inputs[1], b) &&
           parse_uint64(tool->inputs[2], modulus);
}

static void append_digit(calculator_number_theory_t *tool, char digit,
                         char *message, size_t message_size) {
    char *text = tool->inputs[tool->active_input];
    size_t length = strlen(text);
    if (length == 1u && text[0] == '0') length = 0;
    if (length >= 20u) {
        set_message(message, message_size, "INTEGER FULL");
        return;
    }
    char candidate[21];
    if (length) memcpy(candidate, text, length);
    candidate[length] = digit;
    candidate[length + 1u] = '\0';
    uint64_t ignored;
    if (!parse_uint64(candidate, &ignored)) {
        set_message(message, message_size, "UINT64 OVERFLOW");
        return;
    }
    snprintf(text, 21u, "%s", candidate);
    set_message(message, message_size, "READY");
}

static void factor_value(calculator_number_theory_t *tool, uint64_t value,
                         char *message, size_t message_size) {
    uint64_t factors[64];
    bool complete = true;
    size_t count = number_theory_factor(value, factors, 64u, &complete);
    snprintf(tool->operation, sizeof tool->operation, "PRIME FACTORS");
    tool->result[0] = '\0';
    if (value < 2u) {
        snprintf(tool->result, sizeof tool->result, "No factorization");
        set_message(message, message_size, "ENTER N >= 2");
        return;
    }
    size_t used = 0;
    for (size_t i = 0; i < count && used < sizeof tool->result; ++i) {
        char factor[24];
        format_uint64(factor, sizeof factor, factors[i]);
        int written = snprintf(tool->result + used,
                               sizeof tool->result - used,
                               "%s%s", i ? " x " : "", factor);
        if (written < 0 || (size_t)written >= sizeof tool->result - used) {
            complete = false;
            break;
        }
        used += (size_t)written;
    }
    tool->has_result = false;
    set_message(message, message_size,
                complete ? "FACTORIZATION OK" : "PARTIAL FACTORS");
}

calculator_number_theory_output_t calculator_number_theory_activate(
    calculator_number_theory_t *tool, const char *token,
    const char *ans_text, uint64_t *output,
    char *message, size_t message_size) {
    if (!tool || !token) return NUMBER_THEORY_OUTPUT_NONE;
    if (token[0] >= '0' && token[0] <= '9' && token[1] == '\0') {
        append_digit(tool, token[0], message, message_size);
        return NUMBER_THEORY_OUTPUT_NONE;
    }
    if (token[0] >= 'A' && token[0] <= 'C' && token[1] == '\0') {
        tool->active_input = (uint8_t)(token[0] - 'A');
        set_message(message, message_size,
                    tool->active_input == 0u ? "INPUT A" :
                    (tool->active_input == 1u ? "INPUT B" : "MODULUS M"));
        return NUMBER_THEORY_OUTPUT_NONE;
    }
    char *active = tool->inputs[tool->active_input];
    if (strcmp(token, "DEL") == 0) {
        size_t length = strlen(active);
        if (length > 1u) active[length - 1u] = '\0';
        else snprintf(active, 21u, "0");
        set_message(message, message_size, "READY");
        return NUMBER_THEORY_OUTPUT_NONE;
    }
    if (strcmp(token, "AC") == 0) {
        calculator_number_theory_init(tool);
        set_message(message, message_size, "CLEARED");
        return NUMBER_THEORY_OUTPUT_NONE;
    }
    if (strcmp(token, "ANS") == 0) {
        uint64_t value;
        if (parse_uint64(ans_text, &value)) {
            format_uint64(active, 21u, value);
            set_message(message, message_size, "ANS INSERTED");
        } else {
            set_message(message, message_size, "ANS NOT UINT64");
        }
        return NUMBER_THEORY_OUTPUT_NONE;
    }
    if (strcmp(token, "MAX") == 0) {
        format_uint64(active, 21u, UINT64_MAX);
        set_message(message, message_size, "UINT64 MAX");
        return NUMBER_THEORY_OUTPUT_NONE;
    }
    if (strcmp(token, "USE") == 0) {
        if (tool->has_result) {
            format_uint64(active, 21u, tool->last_result);
            set_message(message, message_size, "RESULT INSERTED");
        } else set_message(message, message_size, "NO INTEGER RESULT");
        return NUMBER_THEORY_OUTPUT_NONE;
    }
    if (strcmp(token, "SWAP") == 0) {
        char temporary[21];
        memcpy(temporary, tool->inputs[0], sizeof temporary);
        memcpy(tool->inputs[0], tool->inputs[1], sizeof temporary);
        memcpy(tool->inputs[1], temporary, sizeof temporary);
        set_message(message, message_size, "A B SWAPPED");
        return NUMBER_THEORY_OUTPUT_NONE;
    }
    if (strcmp(token, "TOANS") == 0) {
        if (!tool->has_result) {
            set_message(message, message_size, "NO INTEGER RESULT");
            return NUMBER_THEORY_OUTPUT_NONE;
        }
        if (output) *output = tool->last_result;
        set_message(message, message_size, "RESULT TO ANS");
        return NUMBER_THEORY_OUTPUT_ANS;
    }

    uint64_t a;
    uint64_t b;
    uint64_t modulus;
    if (!read_inputs(tool, &a, &b, &modulus)) {
        set_message(message, message_size, "INVALID UINT64");
        return NUMBER_THEORY_OUTPUT_NONE;
    }
    if (strcmp(token, "GCD") == 0) {
        set_result(tool, "ggT / GCD", number_theory_gcd(a, b));
        set_message(message, message_size, "GCD OK");
    } else if (strcmp(token, "LCM") == 0) {
        uint64_t value;
        if (number_theory_lcm(a, b, &value)) {
            set_result(tool, "kgV / LCM", value);
            set_message(message, message_size, "LCM OK");
        } else set_message(message, message_size, "LCM OVERFLOW");
    } else if (strcmp(token, "PRIME") == 0) {
        tool->has_result = false;
        snprintf(tool->operation, sizeof tool->operation, "PRIME TEST");
        char value[24];
        format_uint64(value, sizeof value, a);
        snprintf(tool->result, sizeof tool->result, "%s is %s", value,
                 number_theory_is_prime(a) ? "PRIME" : "COMPOSITE");
        set_message(message, message_size, "PRIME TEST OK");
    } else if (strcmp(token, "NEXT") == 0 ||
               strcmp(token, "PREV") == 0) {
        uint64_t value;
        bool ok = strcmp(token, "NEXT") == 0
            ? number_theory_next_prime(a, &value)
            : number_theory_previous_prime(a, &value);
        if (ok) {
            set_result(tool, strcmp(token, "NEXT") == 0
                       ? "NEXT PRIME" : "PREVIOUS PRIME", value);
            set_message(message, message_size, "PRIME FOUND");
        } else set_message(message, message_size, "NO PRIME IN RANGE");
    } else if (strcmp(token, "FACTOR") == 0) {
        factor_value(tool, a, message, message_size);
    } else if (strcmp(token, "PHI") == 0) {
        uint64_t value;
        if (number_theory_totient(a, &value)) {
            set_result(tool, "EULER PHI", value);
            set_message(message, message_size, "PHI OK");
        } else set_message(message, message_size, "PHI FAILED");
    } else if (strcmp(token, "MOD") == 0) {
        if (b) {
            set_result(tool, "A mod B", a % b);
            set_message(message, message_size, "MOD OK");
        } else set_message(message, message_size, "DIVISION BY ZERO");
    } else if (strcmp(token, "POW") == 0) {
        if (modulus) {
            set_result(tool, "A^B mod M",
                       number_theory_mod_pow(a, b, modulus));
            set_message(message, message_size, "MOD POW OK");
        } else set_message(message, message_size, "MODULUS ZERO");
    }
    return NUMBER_THEORY_OUTPUT_NONE;
}
