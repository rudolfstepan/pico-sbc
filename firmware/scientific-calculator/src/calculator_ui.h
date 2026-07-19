#pragma once

#include <stddef.h>

void calculator_ui_init(void);
void calculator_ui_task(void);
void calculator_ui_usb_command(const char *command,
                               char *response, size_t response_size);
