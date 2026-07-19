#include "calculator_complex.h"

#include <stdio.h>
#include <string.h>

static void set_message(char *message, size_t size, const char *text) {
    if (!message || !size) return;
    snprintf(message, size, "%s", text);
}

void calculator_complex_init(calculator_complex_t *complex) {
    if (!complex) return;
    memset(complex, 0, sizeof *complex);
    expression_editor_init(&complex->editor);
}

static void add_history(calculator_complex_t *complex) {
    if (complex->history_count == CALCULATOR_COMPLEX_HISTORY_CAPACITY) {
        memmove(&complex->history[0], &complex->history[1],
                (CALCULATOR_COMPLEX_HISTORY_CAPACITY - 1) *
                    sizeof complex->history[0]);
        complex->history_count--;
    }
    calculator_complex_history_entry_t *entry =
        &complex->history[complex->history_count++];
    snprintf(entry->expression, sizeof entry->expression, "%s",
             complex->editor.text);
    entry->result = complex->result;
    complex->history_index = complex->history_count - 1;
}

static bool operator_token(const char *token) {
    return token && token[0] && token[1] == '\0' &&
           strchr("+-*/^", token[0]);
}

static void insert_token(calculator_complex_t *complex, const char *token,
                         char *message, size_t message_size) {
    if (complex->just_evaluated) {
        if (operator_token(token)) {
            char previous[64];
            char grouped[68];
            double real = complex->result.real == 0.0
                ? 0.0 : complex->result.real;
            double imag = complex->result.imag == 0.0
                ? 0.0 : complex->result.imag;
            int written = snprintf(previous, sizeof previous,
                                   "%.17g%+.17gi", real, imag);
            if (written >= 0 && (size_t)written < sizeof previous) {
                snprintf(grouped, sizeof grouped, "(%s)", previous);
                expression_editor_set(&complex->editor, grouped);
            }
        } else {
            expression_editor_clear(&complex->editor);
        }
        complex->just_evaluated = false;
    }
    complex->history_view = false;
    if (expression_editor_insert(&complex->editor, token)) {
        set_message(message, message_size, "EDIT");
    } else {
        set_message(message, message_size, "INPUT FULL");
    }
}

static void evaluate(calculator_complex_t *complex, bool degrees,
                     char *message, size_t message_size) {
    int error_position = 0;
    complex_status_t status = complex_engine_evaluate(
        complex->editor.text, degrees, &complex->result, &error_position);
    if (status == COMPLEX_STATUS_OK) {
        complex->has_result = true;
        complex->just_evaluated = true;
        complex->history_view = false;
        add_history(complex);
        set_message(message, message_size, "COMPLEX OK");
    } else if (status == COMPLEX_STATUS_SYNTAX && error_position > 0) {
        snprintf(message, message_size, "SYNTAX AT %d", error_position);
    } else {
        set_message(message, message_size,
                    complex_engine_status_text(status));
    }
}

static void history_action(calculator_complex_t *complex, const char *token,
                           char *message, size_t message_size) {
    if (strcmp(token, "HIST") == 0) {
        if (!complex->history_count) {
            set_message(message, message_size, "NO HISTORY");
            return;
        }
        complex->history_view = true;
        complex->history_index = complex->history_count - 1;
        set_message(message, message_size, "COMPLEX HISTORY");
    } else if (strcmp(token, "BACK") == 0) {
        complex->history_view = false;
        set_message(message, message_size, "EDIT");
    } else if (strcmp(token, "PREV") == 0) {
        if (complex->history_index) complex->history_index--;
        set_message(message, message_size, "OLDER RESULT");
    } else if (strcmp(token, "NEXT") == 0) {
        if (complex->history_index + 1 < complex->history_count) {
            complex->history_index++;
        }
        set_message(message, message_size, "NEWER RESULT");
    } else if (strcmp(token, "USE") == 0) {
        if (!complex->history_count) {
            set_message(message, message_size, "NO HISTORY");
            return;
        }
        const calculator_complex_history_entry_t *entry =
            &complex->history[complex->history_index];
        expression_editor_set(&complex->editor, entry->expression);
        complex->result = entry->result;
        complex->has_result = true;
        complex->history_view = false;
        complex->just_evaluated = false;
        set_message(message, message_size, "HISTORY LOADED");
    } else if (strcmp(token, "HCLR") == 0) {
        memset(complex->history, 0, sizeof complex->history);
        complex->history_count = 0;
        complex->history_index = 0;
        complex->history_view = false;
        set_message(message, message_size, "HISTORY CLEARED");
    }
}

void calculator_complex_activate(calculator_complex_t *complex,
                                 const char *token, bool degrees,
                                 char *message, size_t message_size) {
    if (!complex || !token) return;
    if (strcmp(token, "=") == 0) {
        evaluate(complex, degrees, message, message_size);
    } else if (strcmp(token, "VIEW") == 0) {
        complex->polar_view = !complex->polar_view;
        set_message(message, message_size,
                    complex->polar_view ? "POLAR VIEW" : "CARTESIAN VIEW");
    } else if (strcmp(token, "DEL") == 0) {
        complex->history_view = false;
        complex->just_evaluated = false;
        expression_editor_delete(&complex->editor);
        set_message(message, message_size, "EDIT");
    } else if (strcmp(token, "AC") == 0) {
        expression_editor_clear(&complex->editor);
        complex->has_result = false;
        complex->history_view = false;
        complex->just_evaluated = false;
        set_message(message, message_size, "CLEARED");
    } else if (strcmp(token, "HIST") == 0 ||
               strcmp(token, "BACK") == 0 ||
               strcmp(token, "PREV") == 0 ||
               strcmp(token, "NEXT") == 0 ||
               strcmp(token, "USE") == 0 ||
               strcmp(token, "HCLR") == 0) {
        history_action(complex, token, message, message_size);
    } else {
        insert_token(complex, token, message, message_size);
    }
}
