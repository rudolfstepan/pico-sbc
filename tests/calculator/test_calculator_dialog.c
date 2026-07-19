#include "calculator_dialog.h"

#include <stdio.h>
#include <string.h>

int main(void) {
    calculator_dialog_t dialog;
    calculator_dialog_init(&dialog);
    if (dialog.visible || dialog.title[0] || dialog.body[0]) return 1;

    calculator_dialog_open(&dialog, CALCULATOR_DIALOG_CONFIRM,
                           "RESET", "Restore factory settings?");
    if (!dialog.visible || dialog.kind != CALCULATOR_DIALOG_CONFIRM ||
        strcmp(dialog.title, "RESET") != 0 ||
        strcmp(dialog.body, "Restore factory settings?") != 0) {
        printf("FAIL: dialog content\n");
        return 1;
    }

    calculator_dialog_close(&dialog);
    if (dialog.visible) {
        printf("FAIL: dialog close\n");
        return 1;
    }
    printf("calculator dialog tests passed\n");
    return 0;
}
