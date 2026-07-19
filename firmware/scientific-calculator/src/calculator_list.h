#ifndef CALCULATOR_LIST_H
#define CALCULATOR_LIST_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    size_t count;
    size_t index;
} calculator_list_t;

void calculator_list_init(calculator_list_t *list);
void calculator_list_set_count(calculator_list_t *list, size_t count);
bool calculator_list_select(calculator_list_t *list, size_t index);
bool calculator_list_previous(calculator_list_t *list);
bool calculator_list_next(calculator_list_t *list);
bool calculator_list_empty(const calculator_list_t *list);

#endif
