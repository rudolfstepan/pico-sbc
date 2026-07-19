#ifndef CALCULATOR_LOGIC_H
#define CALCULATOR_LOGIC_H

#include "expression_editor.h"
#include "logic_engine.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    LOGIC_VIEW_EDIT,
    LOGIC_VIEW_TABLE,
    LOGIC_VIEW_DNF,
    LOGIC_VIEW_KNF,
    LOGIC_VIEW_GATES
} calculator_logic_view_t;

typedef struct {
    expression_editor_t editor;
    logic_program_t program;
    calculator_logic_view_t view;
    uint8_t assignment;
    size_t scroll;
    bool compiled;
    bool canonical;
    char form[LOGIC_FORM_CAPACITY];
} calculator_logic_t;

void calculator_logic_init(calculator_logic_t *logic);
bool calculator_logic_compile(calculator_logic_t *logic,
                              char *message, size_t message_size);
void calculator_logic_activate(calculator_logic_t *logic, const char *token,
                               char *message, size_t message_size);

#endif
