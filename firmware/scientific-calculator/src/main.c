#include "calculator_ui.h"

#include "board.h"
#include "lcd_st7796.h"
#include "touch_gt911.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    board_init();
    lcd_init();
    (void)touch_init();

    calculator_ui_init();

    while (true) {
        calculator_ui_task();
        tight_loop_contents();
        sleep_ms(10);
    }
}
