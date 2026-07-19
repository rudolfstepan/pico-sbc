#ifndef CALCULATOR_PROGRAM_H
#define CALCULATOR_PROGRAM_H

#include "basic_engine.h"
#include "calculator_ui_types.h"
#include "expression_editor.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CALCULATOR_PROGRAM_RENDER 0x01u
#define CALCULATOR_PROGRAM_DIRTY  0x02u
#define CALCULATOR_PROGRAM_EXIT   0x04u
#define CALCULATOR_PROGRAM_BEEP   0x08u

typedef struct {
    basic_engine_t engine;
    expression_editor_t editor;
    size_t list_scroll;
    calculator_layout_t layout;
    bool output_view;
    bool symbol_layer;
    bool clear_armed;
    char notice[24];
} calculator_program_t;

void calculator_program_init(calculator_program_t *program);
void calculator_program_set_source(calculator_program_t *program,
                                   const basic_program_t *source);
void calculator_program_set_layout(calculator_program_t *program,
                                   calculator_layout_t layout);
const basic_program_t *calculator_program_source(
    const calculator_program_t *program);
void calculator_program_render(calculator_program_t *program);
unsigned int calculator_program_touch(calculator_program_t *program,
                                      uint16_t x, uint16_t y);
unsigned int calculator_program_enter(calculator_program_t *program);
unsigned int calculator_program_toggle_view(calculator_program_t *program);
unsigned int calculator_program_move(calculator_program_t *program,
                                     int horizontal, int vertical);
unsigned int calculator_program_task(calculator_program_t *program,
                                     unsigned int step_budget);

#endif
