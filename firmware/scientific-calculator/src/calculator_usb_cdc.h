#ifndef CALCULATOR_USB_CDC_H
#define CALCULATOR_USB_CDC_H

#include <stddef.h>

typedef void (*calculator_usb_command_handler_t)(
    const char *command, char *response, size_t response_size);

void calculator_usb_cdc_init(void);
void calculator_usb_cdc_task(calculator_usb_command_handler_t handler);

#endif
