#include "calculator_usb_cdc.h"

#include "calculator_usb_protocol.h"

#include "pico/stdio.h"
#include "pico/stdio_usb.h"

#include <stdio.h>

#define USB_INPUT_BUDGET_PER_TASK 32u

static calculator_usb_line_reader_t line_reader;
static bool was_connected;

void calculator_usb_cdc_init(void) {
    calculator_usb_line_reader_init(&line_reader);
    was_connected = false;
}

void calculator_usb_cdc_task(calculator_usb_command_handler_t handler) {
    bool connected = stdio_usb_connected();
    if (was_connected && !connected) {
        /* Drop any partial line so it cannot corrupt the first command
         * of the next session. */
        calculator_usb_line_reader_init(&line_reader);
    }
    was_connected = connected;
    if (!handler || !connected) return;
    for (unsigned int count = 0; count < USB_INPUT_BUDGET_PER_TASK; ++count) {
        int character = getchar_timeout_us(0);
        if (character < 0) return;
        calculator_usb_line_status_t status =
            calculator_usb_line_reader_feed(&line_reader, character);
        if (status == CALCULATOR_USB_LINE_NONE) continue;

        char response[CALCULATOR_USB_RESPONSE_CAPACITY];
        if (status == CALCULATOR_USB_LINE_READY) {
            handler(line_reader.line, response, sizeof response);
        } else if (status == CALCULATOR_USB_LINE_TOO_LONG) {
            snprintf(response, sizeof response, "ERR LINE_TOO_LONG");
        } else {
            snprintf(response, sizeof response, "ERR INVALID_CHAR");
        }
        printf("%s\r\n", response);
        return;
    }
}
