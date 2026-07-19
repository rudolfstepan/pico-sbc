#ifndef CALCULATOR_USB_PROTOCOL_H
#define CALCULATOR_USB_PROTOCOL_H

#include "calculator_persistence.h"
#include "expression_editor.h"

#include <stdbool.h>
#include <stddef.h>

#define CALCULATOR_USB_PROTOCOL_VERSION 2u
#define CALCULATOR_USB_LINE_CAPACITY 192u
#define CALCULATOR_USB_RESPONSE_CAPACITY 192u

typedef struct {
    calculator_persisted_state_t *state;
    expression_editor_t *editor;
    basic_engine_t *basic_engine;
} calculator_usb_context_t;

typedef struct {
    bool changed;
    bool evaluated;
    bool persistent_changed;
    bool statistics_changed;
    bool basic_program_changed;
    bool basic_runtime_changed;
} calculator_usb_effect_t;

typedef enum {
    CALCULATOR_USB_LINE_NONE,
    CALCULATOR_USB_LINE_READY,
    CALCULATOR_USB_LINE_TOO_LONG,
    CALCULATOR_USB_LINE_INVALID
} calculator_usb_line_status_t;

typedef struct {
    char line[CALCULATOR_USB_LINE_CAPACITY];
    size_t length;
    bool overflow;
    bool invalid;
} calculator_usb_line_reader_t;

void calculator_usb_line_reader_init(calculator_usb_line_reader_t *reader);
calculator_usb_line_status_t calculator_usb_line_reader_feed(
    calculator_usb_line_reader_t *reader, int character);
void calculator_usb_execute(calculator_usb_context_t *context,
                            const char *line,
                            char *response, size_t response_size,
                            calculator_usb_effect_t *effect);

#endif
