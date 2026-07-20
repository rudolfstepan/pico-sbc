#include "calculator_logic.h"
#include "lcd_st7796.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static void press(calculator_logic_t *logic, const char *token,
                  char *message, size_t message_size) {
    calculator_logic_activate(logic, token, message, message_size);
}

int main(void) {
    calculator_logic_t logic;
    char message[32];
    calculator_logic_init(&logic);

    press(&logic, "A", message, sizeof message);
    press(&logic, "&", message, sizeof message);
    press(&logic, "B", message, sizeof message);
    CHECK(strcmp(logic.editor.text, "A&B") == 0);
    press(&logic, "TABLE", message, sizeof message);
    CHECK(logic.compiled && logic.view == LOGIC_VIEW_TABLE);
    CHECK(logic_engine_truth_row_count(&logic.program) == 4);

    press(&logic, "GATES", message, sizeof message);
    CHECK(logic.view == LOGIC_VIEW_GATES);
    press(&logic, "A", message, sizeof message);
    CHECK(!logic_engine_evaluate(&logic.program, logic.assignment));
    press(&logic, "B", message, sizeof message);
    CHECK(logic_engine_evaluate(&logic.program, logic.assignment));

    press(&logic, "DNF", message, sizeof message);
    CHECK(logic.view == LOGIC_VIEW_DNF && !logic.canonical);
    CHECK(strcmp(logic.form, "(A&B)") == 0);
    press(&logic, "DNF", message, sizeof message);
    CHECK(logic.canonical);
    press(&logic, "USE", message, sizeof message);
    CHECK(logic.view == LOGIC_VIEW_EDIT);
    CHECK(strcmp(logic.editor.text, "(A&B)") == 0);

    press(&logic, "AC", message, sizeof message);
    press(&logic, "A", message, sizeof message);
    press(&logic, " NAND ", message, sizeof message);
    press(&logic, "B", message, sizeof message);
    press(&logic, "CHECK", message, sizeof message);
    CHECK(logic.compiled);
    CHECK(logic_engine_evaluate(&logic.program, 0));
    CHECK(!logic_engine_evaluate(&logic.program, 3));

    char display[64];
    CHECK(calculator_logic_format_display(
        "!A & B OR C XOR D NAND E NOR F IMPLIES A XNOR B",
        display, sizeof display));
    const char expected[] =
        LCD_TEXT_LOGIC_NOT "A " LCD_TEXT_LOGIC_AND " B "
        LCD_TEXT_LOGIC_OR " C " LCD_TEXT_LOGIC_XOR " D "
        LCD_TEXT_LOGIC_NAND " E " LCD_TEXT_LOGIC_NOR " F "
        LCD_TEXT_LOGIC_IMPLIES " A " LCD_TEXT_LOGIC_XNOR " B";
    CHECK(strcmp(display, expected) == 0);

    puts("calculator logic tests passed");
    return 0;
}
