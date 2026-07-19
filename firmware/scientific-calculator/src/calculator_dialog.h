#ifndef CALCULATOR_DIALOG_H
#define CALCULATOR_DIALOG_H

#include <stdbool.h>
#include <stddef.h>

#define CALCULATOR_DIALOG_TITLE_CAPACITY 24
#define CALCULATOR_DIALOG_BODY_CAPACITY 72

typedef enum {
    CALCULATOR_DIALOG_INFO,
    CALCULATOR_DIALOG_CONFIRM
} calculator_dialog_kind_t;

typedef struct {
    calculator_dialog_kind_t kind;
    bool visible;
    char title[CALCULATOR_DIALOG_TITLE_CAPACITY];
    char body[CALCULATOR_DIALOG_BODY_CAPACITY];
} calculator_dialog_t;

void calculator_dialog_init(calculator_dialog_t *dialog);
void calculator_dialog_open(calculator_dialog_t *dialog,
                            calculator_dialog_kind_t kind,
                            const char *title, const char *body);
void calculator_dialog_close(calculator_dialog_t *dialog);

#endif
