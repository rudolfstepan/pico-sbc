#include "calculator_list.h"

void calculator_list_init(calculator_list_t *list) {
    list->count = 0;
    list->index = 0;
}

void calculator_list_set_count(calculator_list_t *list, size_t count) {
    list->count = count;
    if (!count) {
        list->index = 0;
    } else if (list->index >= count) {
        list->index = count - 1;
    }
}

bool calculator_list_select(calculator_list_t *list, size_t index) {
    if (index >= list->count) return false;
    list->index = index;
    return true;
}

bool calculator_list_previous(calculator_list_t *list) {
    if (!list->count || !list->index) return false;
    list->index--;
    return true;
}

bool calculator_list_next(calculator_list_t *list) {
    if (!list->count || list->index + 1 >= list->count) return false;
    list->index++;
    return true;
}

bool calculator_list_empty(const calculator_list_t *list) {
    return !list->count;
}
