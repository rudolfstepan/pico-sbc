#include "calculator_ui.h"
#include "calculator_usb_cdc.h"

#include "board.h"
#include "lcd_st7796.h"
#include "touch_gt911.h"
#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();
    board_init();
    lcd_init();

    /* Keep this short boot marker independent of the UI state. It also makes
     * LCD initialization failures distinguishable from later UI failures. */
    lcd_fill(RGB565(255, 255, 255));
    lcd_fill_rect(8, 8, lcd_width() - 16, lcd_height() - 16,
                  RGB565(0, 0, 0));
    lcd_draw_text(24, 24, "PICO CALCULATOR", RGB565(255, 255, 255),
                  RGB565(0, 0, 0), 3);
    sleep_ms(600);

    (void)touch_init();

    calculator_ui_init();
    calculator_usb_cdc_init();

    while (true) {
        calculator_ui_task();
        calculator_usb_cdc_task(calculator_ui_usb_command);
        tight_loop_contents();
        sleep_ms(10);
    }
}
