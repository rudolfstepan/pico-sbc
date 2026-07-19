#include "calculator_logic.h"

#include <stdio.h>
#include <string.h>

#define FORM_PAGE_CHARS 380u
#define TABLE_PAGE_ROWS 4u

static size_t gate_count(const logic_program_t *program) {
    size_t count = 0;
    for (size_t i = 0; i < program->node_count; ++i) {
        if (program->nodes[i].kind >= LOGIC_NODE_NOT) count++;
    }
    return count;
}

static void set_message(char *message, size_t size, const char *text) {
    if (!message || !size) return;
    snprintf(message, size, "%s", text);
}

void calculator_logic_init(calculator_logic_t *logic) {
    if (!logic) return;
    memset(logic, 0, sizeof *logic);
    expression_editor_init(&logic->editor);
    logic->view = LOGIC_VIEW_EDIT;
}

bool calculator_logic_compile(calculator_logic_t *logic,
                              char *message, size_t message_size) {
    if (!logic) return false;
    int error_position = 0;
    logic_status_t status = logic_engine_compile(
        logic->editor.text, &logic->program, &error_position);
    logic->compiled = status == LOGIC_STATUS_OK;
    if (logic->compiled) {
        logic->assignment &= logic->program.variable_mask;
        snprintf(message, message_size, "%u VAR  %u GATES",
                 (unsigned int)logic_engine_variable_count(&logic->program),
                 (unsigned int)gate_count(&logic->program));
        return true;
    }
    if (status == LOGIC_STATUS_SYNTAX && error_position > 0) {
        snprintf(message, message_size, "SYNTAX AT %d", error_position);
    } else {
        set_message(message, message_size, logic_engine_status_text(status));
    }
    return false;
}

static void show_form(calculator_logic_t *logic, bool dnf,
                      bool toggle, char *message, size_t message_size) {
    if (!calculator_logic_compile(logic, message, message_size)) return;
    calculator_logic_view_t target = dnf ? LOGIC_VIEW_DNF : LOGIC_VIEW_KNF;
    if (logic->view == target && toggle) {
        logic->canonical = !logic->canonical;
    } else {
        logic->canonical = false;
    }
    logic_status_t status = logic->canonical
        ? logic_engine_format_canonical(&logic->program, dnf,
                                        logic->form, sizeof logic->form)
        : logic_engine_format_simplified(&logic->program, dnf,
                                         logic->form, sizeof logic->form);
    if (status != LOGIC_STATUS_OK) {
        set_message(message, message_size, logic_engine_status_text(status));
        return;
    }
    logic->view = target;
    logic->scroll = 0;
    snprintf(message, message_size, "%s %s", dnf ? "DNF" : "KNF",
             logic->canonical ? "CANONICAL" : "SIMPLIFIED");
}

static void scroll_view(calculator_logic_t *logic, bool down) {
    if (logic->view == LOGIC_VIEW_TABLE && logic->compiled) {
        size_t rows = logic_engine_truth_row_count(&logic->program);
        size_t maximum = rows > TABLE_PAGE_ROWS ? rows - TABLE_PAGE_ROWS : 0;
        if (down) {
            logic->scroll = logic->scroll + TABLE_PAGE_ROWS > maximum
                ? maximum : logic->scroll + TABLE_PAGE_ROWS;
        } else {
            logic->scroll = logic->scroll > TABLE_PAGE_ROWS
                ? logic->scroll - TABLE_PAGE_ROWS : 0;
        }
        return;
    }
    if (logic->view != LOGIC_VIEW_DNF && logic->view != LOGIC_VIEW_KNF) return;
    size_t length = strlen(logic->form);
    size_t maximum = length > FORM_PAGE_CHARS
        ? length - FORM_PAGE_CHARS : 0;
    if (down) {
        logic->scroll = logic->scroll + FORM_PAGE_CHARS > maximum
            ? maximum : logic->scroll + FORM_PAGE_CHARS;
    } else {
        logic->scroll = logic->scroll > FORM_PAGE_CHARS
            ? logic->scroll - FORM_PAGE_CHARS : 0;
    }
}

static bool is_variable_token(const char *token) {
    return token && token[0] >= 'A' && token[0] <= 'F' && token[1] == '\0';
}

static void insert_logic_token(calculator_logic_t *logic, const char *token,
                               char *message, size_t message_size) {
    logic->view = LOGIC_VIEW_EDIT;
    logic->compiled = false;
    logic->scroll = 0;
    if (expression_editor_insert(&logic->editor, token)) {
        set_message(message, message_size, "EDIT");
    } else {
        set_message(message, message_size, "INPUT FULL");
    }
}

void calculator_logic_activate(calculator_logic_t *logic, const char *token,
                               char *message, size_t message_size) {
    if (!logic || !token) return;
    if (strcmp(token, "CHECK") == 0) {
        logic->view = LOGIC_VIEW_EDIT;
        calculator_logic_compile(logic, message, message_size);
    } else if (strcmp(token, "TABLE") == 0) {
        if (calculator_logic_compile(logic, message, message_size)) {
            logic->view = LOGIC_VIEW_TABLE;
            logic->scroll = 0;
            set_message(message, message_size, "TRUTH TABLE");
        }
    } else if (strcmp(token, "DNF") == 0) {
        show_form(logic, true, true, message, message_size);
    } else if (strcmp(token, "KNF") == 0) {
        show_form(logic, false, true, message, message_size);
    } else if (strcmp(token, "GATES") == 0) {
        if (calculator_logic_compile(logic, message, message_size)) {
            logic->view = LOGIC_VIEW_GATES;
            logic->scroll = 0;
            set_message(message, message_size, "TOUCH INPUTS A-F");
        }
    } else if (strcmp(token, "DEL") == 0) {
        logic->view = LOGIC_VIEW_EDIT;
        logic->compiled = false;
        expression_editor_delete(&logic->editor);
        set_message(message, message_size, "EDIT");
    } else if (strcmp(token, "AC") == 0) {
        calculator_logic_init(logic);
        set_message(message, message_size, "CLEARED");
    } else if (strcmp(token, "LEFT") == 0 ||
               strcmp(token, "RIGHT") == 0) {
        logic->view = LOGIC_VIEW_EDIT;
        expression_editor_move(&logic->editor,
            strcmp(token, "LEFT") == 0
                ? EDITOR_CURSOR_LEFT : EDITOR_CURSOR_RIGHT);
        set_message(message, message_size, "EDIT");
    } else if (strcmp(token, "UP") == 0 || strcmp(token, "DOWN") == 0) {
        scroll_view(logic, strcmp(token, "DOWN") == 0);
        set_message(message, message_size, "SCROLLED");
    } else if (strcmp(token, "USE") == 0) {
        if ((logic->view == LOGIC_VIEW_DNF || logic->view == LOGIC_VIEW_KNF) &&
            expression_editor_set(&logic->editor, logic->form)) {
            logic->view = LOGIC_VIEW_EDIT;
            logic->compiled = false;
            logic->scroll = 0;
            set_message(message, message_size, "FORM LOADED");
        } else if (logic->view == LOGIC_VIEW_DNF ||
                   logic->view == LOGIC_VIEW_KNF) {
            set_message(message, message_size, "FORM TOO LONG");
        } else {
            set_message(message, message_size, "SELECT DNF OR KNF");
        }
    } else if (logic->view == LOGIC_VIEW_GATES &&
               is_variable_token(token)) {
        uint8_t bit = (uint8_t)(1u << (token[0] - 'A'));
        if (logic->program.variable_mask & bit) {
            logic->assignment ^= bit;
            snprintf(message, message_size, "%c = %u", token[0],
                     (logic->assignment & bit) ? 1u : 0u);
        } else {
            set_message(message, message_size, "INPUT UNUSED");
        }
    } else {
        insert_logic_token(logic, token, message, message_size);
    }
}
