#include "board.h"
#include "lcd_st7796.h"
#include "touch_gt911.h"
#include "ui.h"

#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    board_init();
    lcd_init();
    (void)touch_init();

    board_led1(true);
    ui_init();
    board_beep(880, 80);
    board_led1(false);

    while (true) {
        ui_task();
        tight_loop_contents();
        sleep_ms(10);
    }
}
