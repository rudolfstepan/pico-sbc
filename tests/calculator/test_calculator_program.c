#include "calculator_program.h"
#include "mock_lcd.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "check failed at line %d: %s\n", \
                __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    calculator_program_t program;
    calculator_program_init(&program);

    mock_lcd_reset();
    calculator_program_render(&program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());
    CHECK(mock_lcd_max_text_scale() >= 2);

    CHECK(calculator_program_touch(&program, 12, 145) &
          CALCULATOR_PROGRAM_RENDER);
    CHECK(calculator_program_touch(&program, 430, 145) &
          CALCULATOR_PROGRAM_RENDER);
    CHECK(strcmp(program.editor.text, "10") == 0);
    calculator_program_touch(&program, 150, 280);
    calculator_program_touch(&program, 90, 280);
    CHECK(program.symbol_layer);
    calculator_program_touch(&program, 60, 180);
    CHECK(strcmp(program.editor.text, "10 PRINT ") == 0);

    unsigned int effects = calculator_program_touch(&program, 410, 220);
    CHECK(effects & CALCULATOR_PROGRAM_DIRTY);
    CHECK(program.engine.program.count == 1);
    CHECK(strcmp(program.engine.program.lines[0].text, "PRINT") == 0);

    calculator_program_touch(&program, 320, 280);
    CHECK(program.engine.state == BASIC_RUN_RUNNING);
    CHECK(calculator_program_task(&program, 16) & CALCULATOR_PROGRAM_RENDER);
    CHECK(program.engine.state == BASIC_RUN_FINISHED);
    CHECK(program.engine.output_count == 1);
    calculator_program_render(&program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());

    calculator_program_touch(&program, 20, 280);
    CHECK(program.clear_armed && program.engine.program.count == 1);
    effects = calculator_program_touch(&program, 20, 280);
    CHECK((effects & CALCULATOR_PROGRAM_DIRTY) &&
          program.engine.program.count == 0);
    CHECK(calculator_program_touch(&program, 400, 280) &
          CALCULATOR_PROGRAM_EXIT);

    puts("calculator program tests passed");
    return 0;
}
