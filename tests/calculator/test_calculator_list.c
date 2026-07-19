#include "calculator_list.h"

#include <stdio.h>

static int failures;

static void expect_state(const calculator_list_t *list,
                         size_t count, size_t index) {
    if (list->count != count || list->index != index) {
        printf("FAIL: count=%u index=%u, expected count=%u index=%u\n",
               (unsigned int)list->count, (unsigned int)list->index,
               (unsigned int)count, (unsigned int)index);
        failures++;
    }
}

int main(void) {
    calculator_list_t list;
    calculator_list_init(&list);
    expect_state(&list, 0, 0);
    if (!calculator_list_empty(&list)) failures++;
    if (calculator_list_previous(&list) || calculator_list_next(&list)) {
        failures++;
    }

    calculator_list_set_count(&list, 3);
    if (!calculator_list_select(&list, 2)) failures++;
    if (calculator_list_select(&list, 3)) failures++;
    expect_state(&list, 3, 2);
    if (calculator_list_next(&list)) failures++;
    if (!calculator_list_previous(&list)) failures++;
    expect_state(&list, 3, 1);

    calculator_list_set_count(&list, 1);
    expect_state(&list, 1, 0);
    calculator_list_set_count(&list, 0);
    expect_state(&list, 0, 0);

    if (failures) {
        printf("%d calculator list test(s) failed\n", failures);
        return 1;
    }
    printf("calculator list tests passed\n");
    return 0;
}
