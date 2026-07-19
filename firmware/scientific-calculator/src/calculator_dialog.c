#include "calculator_dialog.h"

#include <stdio.h>

void calculator_dialog_init(calculator_dialog_t *dialog) {
    dialog->kind = CALCULATOR_DIALOG_INFO;
    dialog->visible = false;
    dialog->title[0] = '\0';
    dialog->body[0] = '\0';
}

void calculator_dialog_open(calculator_dialog_t *dialog,
                            calculator_dialog_kind_t kind,
                            const char *title, const char *body) {
    dialog->kind = kind;
    dialog->visible = true;
    snprintf(dialog->title, sizeof dialog->title, "%s", title ? title : "");
    snprintf(dialog->body, sizeof dialog->body, "%s", body ? body : "");
}

void calculator_dialog_close(calculator_dialog_t *dialog) {
    dialog->visible = false;
}
