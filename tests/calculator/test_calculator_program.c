#include "calculator_program.h"
#include "lcd_st7796.h"
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
    calculator_program_touch(&program, 170, 245);
    calculator_program_touch(&program, 170, 245);
    CHECK(strcmp(program.editor.text, "10 PRINT \"\"") == 0);

    unsigned int effects = calculator_program_touch(&program, 410, 220);
    CHECK(effects & CALCULATOR_PROGRAM_DIRTY);
    CHECK(program.engine.program.count == 1);
    CHECK(strcmp(program.engine.program.lines[0].text, "PRINT \"\"") == 0);

    calculator_program_touch(&program, 320, 280);
    CHECK(program.engine.state == BASIC_RUN_RUNNING);
    CHECK(calculator_program_task(&program, 16) & CALCULATOR_PROGRAM_RENDER);
    CHECK(program.engine.state == BASIC_RUN_FINISHED);
    CHECK(program.engine.output_count == 1);
    calculator_program_render(&program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());

    calculator_program_t wrapped_program;
    calculator_program_init(&wrapped_program);
    wrapped_program.output_view = true;
    wrapped_program.engine.output_count = 1;
    snprintf(wrapped_program.engine.output[0], BASIC_OUTPUT_CAPACITY,
             "ALPHA BETA GAMMA DELTA EPSILON ZETA THETALONG");
    mock_lcd_reset();
    calculator_program_render(&wrapped_program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());
    CHECK(mock_lcd_drew_text("ALPHA BETA GAMMA DELTA EPSILON ZETA"));
    CHECK(mock_lcd_drew_text("THETALONG"));

    calculator_program_touch(&program, 20, 280);
    CHECK(program.clear_armed && program.engine.program.count == 1);
    effects = calculator_program_touch(&program, 20, 280);
    CHECK((effects & CALCULATOR_PROGRAM_DIRTY) &&
          program.engine.program.count == 0);
    CHECK(calculator_program_touch(&program, 400, 280) &
          CALCULATOR_PROGRAM_EXIT);

    calculator_program_t layout_program;
    calculator_program_init(&layout_program);
    calculator_program_set_layout(&layout_program,
                                  CALCULATOR_LAYOUT_DATA_FOCUS);
    mock_lcd_reset();
    calculator_program_render(&layout_program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());
    CHECK(calculator_program_touch(&layout_program, 170, 192) &
          CALCULATOR_PROGRAM_RENDER);
    CHECK(layout_program.symbol_layer);

    calculator_program_set_layout(&layout_program,
                                  CALCULATOR_LAYOUT_FULLSCREEN);
    layout_program.output_view = true;
    layout_program.engine.output_count = BASIC_OUTPUT_LINES;
    for (size_t i = 0; i < BASIC_OUTPUT_LINES; ++i) {
        snprintf(layout_program.engine.output[i], BASIC_OUTPUT_CAPACITY,
                 "LINE %u", (unsigned int)i);
    }
    mock_lcd_reset();
    calculator_program_render(&layout_program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());
    CHECK(mock_lcd_drew_text("LINE 15"));

    size_t editor_length = layout_program.editor.length;
    CHECK(calculator_program_touch(&layout_program, 12, 192) &
          CALCULATOR_PROGRAM_RENDER);
    CHECK(layout_program.editor.length == editor_length);
    CHECK(!layout_program.output_view);

    lcd_set_orientation(LCD_ORIENTATION_PORTRAIT);
    calculator_program_t portrait_program;
    calculator_program_init(&portrait_program);
    mock_lcd_reset();
    calculator_program_render(&portrait_program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());
    portrait_program.symbol_layer = true;
    mock_lcd_reset();
    calculator_program_render(&portrait_program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());
    portrait_program.symbol_layer = false;
    CHECK(calculator_program_touch(&portrait_program, 8, 210) &
          CALCULATOR_PROGRAM_RENDER);
    CHECK(strcmp(portrait_program.editor.text, "1") == 0);

    portrait_program.output_view = true;
    portrait_program.engine.output_count = 1;
    snprintf(portrait_program.engine.output[0], BASIC_OUTPUT_CAPACITY,
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890abcdefghij");
    mock_lcd_reset();
    calculator_program_render(&portrait_program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());
    CHECK(mock_lcd_drew_text("ABCDEFGHIJKLMNOPQRSTUVWXY"));
    CHECK(mock_lcd_drew_text("Z1234567890abcdefghij"));
    portrait_program.output_view = false;

    calculator_program_set_layout(&portrait_program,
                                  CALCULATOR_LAYOUT_DATA_FOCUS);
    mock_lcd_reset();
    calculator_program_render(&portrait_program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());
    CHECK(calculator_program_touch(&portrait_program, 120, 332) &
          CALCULATOR_PROGRAM_RENDER);
    CHECK(portrait_program.symbol_layer);

    calculator_program_set_layout(&portrait_program,
                                  CALCULATOR_LAYOUT_FULLSCREEN);
    portrait_program.output_view = true;
    portrait_program.engine.output_count = BASIC_OUTPUT_LINES;
    for (size_t i = 0; i < BASIC_OUTPUT_LINES; ++i) {
        snprintf(portrait_program.engine.output[i], BASIC_OUTPUT_CAPACITY,
                 "PORTRAIT LINE %u", (unsigned int)i);
    }
    mock_lcd_reset();
    calculator_program_render(&portrait_program);
    CHECK(!mock_lcd_had_out_of_bounds_draw());
    CHECK(mock_lcd_drew_text("PORTRAIT LINE 15"));
    lcd_set_orientation(LCD_ORIENTATION_LANDSCAPE);

    puts("calculator program tests passed");
    return 0;
}
