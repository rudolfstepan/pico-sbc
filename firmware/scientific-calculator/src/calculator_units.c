#include "calculator_units.h"

#include <stdio.h>
#include <string.h>

static void set_message(char *message, size_t size, const char *text) {
    if (!message || !size) return;
    snprintf(message, size, "%s", text);
}

static bool convert_value(calculator_units_t *units,
                          char *message, size_t message_size) {
    if (!units->has_input) {
        units->has_result = false;
        set_message(message, message_size, "PRESS ANS>IN");
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

static void set_category(calculator_units_t *units,
                         unit_category_t category,
                         char *message, size_t message_size) {
    units->category = category;
    units->from_index = 0;
    units->to_index = unit_engine_unit_count(category) > 1 ? 1 : 0;
    units->view = UNITS_VIEW_CONVERTER;
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
    if (category_from_token(token, &category)) {
        set_category(units, category, message, message_size);
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
        if (units->has_result) units->input = units->result;
        units->view = UNITS_VIEW_CONVERTER;
        convert_value(units, message, message_size);
    } else if (strcmp(token, "ANSIN") == 0) {
        units->input = ans;
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
