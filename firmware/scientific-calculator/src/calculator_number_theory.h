#ifndef CALCULATOR_NUMBER_THEORY_H
#define CALCULATOR_NUMBER_THEORY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    NUMBER_THEORY_OUTPUT_NONE,
    NUMBER_THEORY_OUTPUT_ANS
} calculator_number_theory_output_t;

typedef struct {
    char inputs[3][21];
    uint8_t active_input;
    uint64_t last_result;
    bool has_result;
    char operation[24];
    char result[192];
} calculator_number_theory_t;

void calculator_number_theory_init(calculator_number_theory_t *tool);
calculator_number_theory_output_t calculator_number_theory_activate(
    calculator_number_theory_t *tool, const char *token,
    const char *ans_text, uint64_t *output,
    char *message, size_t message_size);

#endif
