#include "calculator_units.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void set_message(char *message, size_t size, const char *text) {
    if (!message || !size) return;
    snprintf(message, size, "%s", text);
}

static bool convert_value(calculator_units_t *units,
                          char *message, size_t message_size) {
    if (!units->has_input) {
        units->has_result = false;
        set_message(message, message_size, "ENTER VALUE");
        return false;
    }
    const unit_definition_t *from =
        unit_engine_unit(units->category, units->from_index);
    const unit_definition_t *to =
        unit_engine_unit(units->category, units->to_index);
    unit_status_t status = unit_engine_convert(units->input, from, to,
                                               &units->result);
    units->has_result = status == UNIT_STATUS_OK;
    set_message(message, message_size, unit_engine_status_text(status));
    return units->has_result;
}

static void format_input(calculator_units_t *units, double value) {
    snprintf(units->input_text, sizeof units->input_text, "%.17g", value);
}

static bool parse_input(calculator_units_t *units) {
    char *end = NULL;
    double value = strtod(units->input_text, &end);
    if (!units->input_text[0] || end == units->input_text || *end != '\0' ||
        !isfinite(value)) {
        units->has_input = false;
        units->has_result = false;
        return false;
    }
    units->input = value;
    units->has_input = true;
    return true;
}

static void refresh_input(calculator_units_t *units,
                          char *message, size_t message_size) {
    units->view = UNITS_VIEW_CONVERTER;
    units->selector = UNITS_SELECTOR_NONE;
    if (parse_input(units)) {
        convert_value(units, message, message_size);
    } else {
        set_message(message, message_size, "ENTER VALUE");
    }
}

static bool append_input(calculator_units_t *units, char character) {
    size_t length = strlen(units->input_text);
    if (length + 1u >= sizeof units->input_text) return false;
    units->input_text[length] = character;
    units->input_text[length + 1u] = '\0';
    return true;
}

static bool insert_input(calculator_units_t *units, size_t position,
                         char character) {
    size_t length = strlen(units->input_text);
    if (position > length || length + 1u >= sizeof units->input_text) {
        return false;
    }
    memmove(units->input_text + position + 1u,
            units->input_text + position, length - position + 1u);
    units->input_text[position] = character;
    return true;
}

static void toggle_input_sign(calculator_units_t *units,
                              char *message, size_t message_size) {
    char *exponent = strchr(units->input_text, 'e');
    size_t position = exponent
        ? (size_t)(exponent - units->input_text) + 1u : 0u;
    if (units->input_text[position] == '-') {
        memmove(units->input_text + position,
                units->input_text + position + 1u,
                strlen(units->input_text + position + 1u) + 1u);
    } else if (!insert_input(units, position, '-')) {
        set_message(message, message_size, "INPUT FULL");
        return;
    }
    refresh_input(units, message, message_size);
    if (exponent && !units->has_input) {
        set_message(message, message_size, "ENTER EXPONENT");
    }
}

static bool direct_input_token(const char *token) {
    return (token[0] >= '0' && token[0] <= '9' && token[1] == '\0') ||
           strcmp(token, ".") == 0 || strcmp(token, "SIGN") == 0 ||
           strcmp(token, "EXP") == 0 || strcmp(token, "DEL") == 0 ||
           strcmp(token, "AC") == 0;
}

static void edit_input(calculator_units_t *units, const char *token,
                       char *message, size_t message_size) {
    if (token[0] >= '0' && token[0] <= '9' && token[1] == '\0') {
        if (!append_input(units, token[0])) {
            set_message(message, message_size, "INPUT FULL");
            return;
        }
    } else if (strcmp(token, ".") == 0) {
        if (strchr(units->input_text, 'e') ||
            strchr(units->input_text, '.')) {
            set_message(message, message_size, "DECIMAL POINT EXISTS");
            return;
        }
        if (!units->input_text[0]) {
            snprintf(units->input_text, sizeof units->input_text, "0.");
        } else if (strcmp(units->input_text, "-") == 0) {
            snprintf(units->input_text, sizeof units->input_text, "-0.");
        } else if (!append_input(units, '.')) {
            set_message(message, message_size, "INPUT FULL");
            return;
        }
    } else if (strcmp(token, "EXP") == 0) {
        if (strchr(units->input_text, 'e')) {
            set_message(message, message_size, "EXPONENT EXISTS");
            return;
        }
        if (!parse_input(units)) {
            set_message(message, message_size, "ENTER MANTISSA");
            return;
        }
        if (!append_input(units, 'e')) {
            set_message(message, message_size, "INPUT FULL");
            return;
        }
    } else if (strcmp(token, "SIGN") == 0) {
        toggle_input_sign(units, message, message_size);
        return;
    } else if (strcmp(token, "DEL") == 0) {
        size_t length = strlen(units->input_text);
        if (length) units->input_text[length - 1u] = '\0';
    } else if (strcmp(token, "AC") == 0) {
        units->input_text[0] = '\0';
    }

    refresh_input(units, message, message_size);
    size_t length = strlen(units->input_text);
    if (length && (units->input_text[length - 1u] == 'e' ||
                   (length >= 2u && units->input_text[length - 2u] == 'e' &&
                    units->input_text[length - 1u] == '-'))) {
        set_message(message, message_size, "ENTER EXPONENT");
    }
}

static void set_category(calculator_units_t *units,
                         unit_category_t category,
                         char *message, size_t message_size) {
    units->category = category;
    units->from_index = 0;
    units->to_index = unit_engine_unit_count(category) > 1 ? 1 : 0;
    units->view = UNITS_VIEW_CONVERTER;
    units->selector = UNITS_SELECTOR_NONE;
    units->selector_offset = 0;
    convert_value(units, message, message_size);
    if (!units->has_input) {
        set_message(message, message_size,
                    unit_engine_category_name(category));
    }
}

static bool category_from_token(const char *token,
                                unit_category_t *category) {
    for (unit_category_t current = UNIT_CATEGORY_LENGTH;
         current < UNIT_CATEGORY_COUNT; ++current) {
        if (strcmp(token, unit_engine_category_name(current)) == 0) {
            *category = current;
            return true;
        }
    }
    return false;
}

static size_t wrapped_index(size_t index, int delta, size_t count) {
    if (!count) return 0;
    if (delta < 0) return index ? index - 1 : count - 1;
    return index + 1 < count ? index + 1 : 0;
}

static void select_unit(calculator_units_t *units, bool from, int delta,
                        char *message, size_t message_size) {
    size_t count = unit_engine_unit_count(units->category);
    size_t *index = from ? &units->from_index : &units->to_index;
    *index = wrapped_index(*index, delta, count);
    units->view = UNITS_VIEW_CONVERTER;
    convert_value(units, message, message_size);
}

static void open_selector(calculator_units_t *units,
                          calculator_units_selector_t selector,
                          char *message, size_t message_size) {
    size_t selected = selector == UNITS_SELECTOR_FROM
        ? units->from_index : units->to_index;
    if (units->selector == selector) {
        size_t count = unit_engine_unit_count(units->category);
        units->selector_offset = count
            ? (units->selector_offset + 6u) % count : 0u;
    } else {
        units->selector = selector;
        units->selector_offset = selected / 6u * 6u;
    }
    units->view = UNITS_VIEW_CONVERTER;
    set_message(message, message_size,
                selector == UNITS_SELECTOR_FROM
                    ? "SELECT SOURCE UNIT" : "SELECT TARGET UNIT");
}

void calculator_units_init(calculator_units_t *units) {
    if (!units) return;
    memset(units, 0, sizeof *units);
    units->category = UNIT_CATEGORY_LENGTH;
    units->from_index = 2;
    units->to_index = 3;
    units->view = UNITS_VIEW_CONVERTER;
}

calculator_units_output_t calculator_units_activate(
    calculator_units_t *units, const char *token, double ans,
    double *output, char *message, size_t message_size) {
    if (!units || !token) return UNITS_OUTPUT_NONE;
    unit_category_t category;
    if (direct_input_token(token)) {
        edit_input(units, token, message, message_size);
    } else if (category_from_token(token, &category)) {
        set_category(units, category, message, message_size);
    } else if (strcmp(token, "FROMSEL") == 0) {
        open_selector(units, UNITS_SELECTOR_FROM, message, message_size);
    } else if (strcmp(token, "TOSEL") == 0) {
        open_selector(units, UNITS_SELECTOR_TO, message, message_size);
    } else if (token[0] == 'U' && token[1] >= '0' && token[1] <= '5' &&
               token[2] == '\0') {
        size_t selected = units->selector_offset +
                          (size_t)(token[1] - '0');
        if (units->view == UNITS_VIEW_CONSTANTS) {
            if (selected < unit_engine_constant_count()) {
                units->constant_index = selected;
                set_message(message, message_size, "CONSTANT SELECTED");
            }
            return UNITS_OUTPUT_NONE;
        }
        size_t count = unit_engine_unit_count(units->category);
        if (selected < count) {
            if (units->selector == UNITS_SELECTOR_TO) {
                units->to_index = selected;
            } else {
                units->from_index = selected;
            }
            units->selector = UNITS_SELECTOR_NONE;
            convert_value(units, message, message_size);
            if (!units->has_input) set_message(message, message_size,
                                               "UNIT SELECTED");
        }
    } else if (strcmp(token, "CAT-") == 0 || strcmp(token, "CAT+") == 0) {
        int next = (int)units->category + (token[3] == '-' ? -1 : 1);
        if (next < 0) next = UNIT_CATEGORY_COUNT - 1;
        if (next >= UNIT_CATEGORY_COUNT) next = 0;
        set_category(units, (unit_category_t)next, message, message_size);
    } else if (strcmp(token, "FROM-") == 0 ||
               strcmp(token, "FROM+") == 0) {
        select_unit(units, true, token[4] == '-' ? -1 : 1,
                    message, message_size);
    } else if (strcmp(token, "TO-") == 0 || strcmp(token, "TO+") == 0) {
        select_unit(units, false, token[2] == '-' ? -1 : 1,
                    message, message_size);
    } else if (strcmp(token, "SWAP") == 0) {
        size_t temporary = units->from_index;
        units->from_index = units->to_index;
        units->to_index = temporary;
        if (units->has_result) {
            units->input = units->result;
            format_input(units, units->input);
        }
        units->view = UNITS_VIEW_CONVERTER;
        convert_value(units, message, message_size);
    } else if (strcmp(token, "ANSIN") == 0) {
        units->input = ans;
        format_input(units, ans);
        units->has_input = true;
        units->view = UNITS_VIEW_CONVERTER;
        convert_value(units, message, message_size);
    } else if (strcmp(token, "CONVERT") == 0) {
        units->view = UNITS_VIEW_CONVERTER;
        convert_value(units, message, message_size);
    } else if (strcmp(token, "CONV") == 0) {
        units->view = UNITS_VIEW_CONVERTER;
        set_message(message, message_size, "CONVERTER");
    } else if (strcmp(token, "CONST") == 0) {
        if (units->view == UNITS_VIEW_CONSTANTS) {
            size_t count = unit_engine_constant_count();
            units->selector_offset = count
                ? (units->selector_offset + 6u) % count : 0u;
        } else {
            units->selector_offset = units->constant_index / 6u * 6u;
        }
        units->view = UNITS_VIEW_CONSTANTS;
        set_message(message, message_size, "CONSTANTS");
    } else if (strcmp(token, "C-") == 0 || strcmp(token, "C+") == 0) {
        units->constant_index = wrapped_index(
            units->constant_index, token[1] == '-' ? -1 : 1,
            unit_engine_constant_count());
        units->view = UNITS_VIEW_CONSTANTS;
        set_message(message, message_size, "CONSTANT SELECTED");
    } else if (strcmp(token, "INFO") == 0) {
        const physical_constant_t *constant =
            unit_engine_constant(units->constant_index);
        units->view = UNITS_VIEW_CONSTANTS;
        set_message(message, message_size,
                    constant ? constant->source : "NO CONSTANT");
    } else if (strcmp(token, "RESET") == 0) {
        calculator_units_init(units);
        set_message(message, message_size, "RESET");
    } else if (strcmp(token, "OUTANS") == 0 ||
               strcmp(token, "OUTEDIT") == 0) {
        if (!units->has_result) {
            set_message(message, message_size, "NO RESULT");
            return UNITS_OUTPUT_NONE;
        }
        if (output) *output = units->result;
        set_message(message, message_size,
                    token[3] == 'A' ? "RESULT TO ANS" : "RESULT TO EDITOR");
        return token[3] == 'A' ? UNITS_OUTPUT_ANS : UNITS_OUTPUT_EDITOR;
    } else if (strcmp(token, "CANS") == 0 || strcmp(token, "CEDIT") == 0) {
        const physical_constant_t *constant =
            unit_engine_constant(units->constant_index);
        if (!constant) {
            set_message(message, message_size, "NO CONSTANT");
            return UNITS_OUTPUT_NONE;
        }
        if (output) *output = constant->value;
        set_message(message, message_size,
                    token[1] == 'A' ? "CONSTANT TO ANS" :
                                      "CONSTANT TO EDITOR");
        return token[1] == 'A' ? UNITS_OUTPUT_ANS : UNITS_OUTPUT_EDITOR;
    }
    return UNITS_OUTPUT_NONE;
}
