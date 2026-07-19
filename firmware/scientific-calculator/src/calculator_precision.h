#ifndef CALCULATOR_PRECISION_H
#define CALCULATOR_PRECISION_H

#include <stdbool.h>

typedef enum {
    CALCULATOR_PRECISION_NORMAL,
    CALCULATOR_PRECISION_HIGH,
    CALCULATOR_PRECISION_ULTRA,
    CALCULATOR_PRECISION_COUNT
} calculator_precision_t;

unsigned int calculator_precision_digits(calculator_precision_t precision);
unsigned int calculator_precision_work_bits(calculator_precision_t precision);
const char *calculator_precision_name(calculator_precision_t precision);
const char *calculator_precision_label(calculator_precision_t precision);
bool calculator_precision_parse(const char *text,
                                calculator_precision_t *precision);

#endif
