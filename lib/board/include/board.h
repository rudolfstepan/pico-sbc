#pragma once

#include <stdbool.h>
#include <stdint.h>

#define PIN_LCD_SCK       2
#define PIN_LCD_MOSI      3
#define PIN_LCD_MISO      4
#define PIN_LCD_CS        5
#define PIN_LCD_DC        6
#define PIN_LCD_RST       7

#define PIN_TOUCH_SDA     8
#define PIN_TOUCH_SCL     9
#define PIN_TOUCH_RST     10
#define PIN_TOUCH_INT     11

#define PIN_WS2812        12
#define PIN_BUZZER        13
#define PIN_BTN2          14
#define PIN_BTN1          15
#define PIN_LED1          16
#define PIN_LED2          17

#define PIN_JOY_X         26
#define PIN_JOY_Y         27
#define ADC_JOY_X         0
#define ADC_JOY_Y         1

typedef struct {
    uint16_t x;
    uint16_t y;
    bool left;
    bool right;
    bool up;
    bool down;
} joystick_state_t;

void board_init(void);
bool board_button1_pressed(void);
bool board_button2_pressed(void);
joystick_state_t board_read_joystick(void);
void board_led1(bool on);
void board_led2(bool on);
void board_beep(uint16_t frequency_hz, uint16_t duration_ms);
