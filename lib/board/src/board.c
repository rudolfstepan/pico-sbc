#include "board.h"

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"

static uint slice_buzzer;
static uint channel_buzzer;

static void buzzer_idle(void) {
    pwm_set_enabled(slice_buzzer, false);
    pwm_set_chan_level(slice_buzzer, channel_buzzer, 0);
    gpio_set_function(PIN_BUZZER, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_BUZZER, GPIO_OUT);
    gpio_put(PIN_BUZZER, 0);
}

void board_init(void) {
    gpio_init(PIN_BTN1);
    gpio_set_dir(PIN_BTN1, GPIO_IN);
    gpio_pull_up(PIN_BTN1);

    gpio_init(PIN_BTN2);
    gpio_set_dir(PIN_BTN2, GPIO_IN);
    gpio_pull_up(PIN_BTN2);

    gpio_init(PIN_LED1);
    gpio_set_dir(PIN_LED1, GPIO_OUT);
    gpio_init(PIN_LED2);
    gpio_set_dir(PIN_LED2, GPIO_OUT);

    adc_init();
    adc_gpio_init(PIN_JOY_X);
    adc_gpio_init(PIN_JOY_Y);

    gpio_init(PIN_BUZZER);
    gpio_set_dir(PIN_BUZZER, GPIO_OUT);
    gpio_put(PIN_BUZZER, 0);
    slice_buzzer = pwm_gpio_to_slice_num(PIN_BUZZER);
    channel_buzzer = pwm_gpio_to_channel(PIN_BUZZER);
    buzzer_idle();
}

bool board_button1_pressed(void) {
    return !gpio_get(PIN_BTN1);
}

bool board_button2_pressed(void) {
    return !gpio_get(PIN_BTN2);
}

joystick_state_t board_read_joystick(void) {
    adc_select_input(ADC_JOY_X);
    uint16_t x = adc_read();
    adc_select_input(ADC_JOY_Y);
    uint16_t y = adc_read();

    joystick_state_t joy = {
        .x = x,
        .y = y,
        .left = x < 1400,
        .right = x > 2700,
        .up = y < 1400,
        .down = y > 2700,
    };
    return joy;
}

void board_led1(bool on) {
    gpio_put(PIN_LED1, on);
}

void board_led2(bool on) {
    gpio_put(PIN_LED2, on);
}

void board_beep(uint16_t frequency_hz, uint16_t duration_ms) {
    if (frequency_hz == 0 || duration_ms == 0) {
        return;
    }

    uint32_t clock = 125000000u;
    uint32_t divider16 = clock / frequency_hz / 4096u + 1u;
    /* pwm_set_clkdiv_int_frac needs an integer part in 1..255: below 16
     * the integer part would be 0 (invalid), above 4095 it would be
     * silently truncated through the uint8_t parameter. */
    if (divider16 < 16u) divider16 = 16u;
    if (divider16 > 4095u) divider16 = 4095u;
    uint32_t wrap = clock * 16u / divider16 / frequency_hz - 1u;
    if (wrap > 65535u) {
        wrap = 65535u;
    }

    gpio_set_function(PIN_BUZZER, GPIO_FUNC_PWM);
    pwm_set_clkdiv_int_frac(slice_buzzer, divider16 / 16u, divider16 & 0x0fu);
    pwm_set_wrap(slice_buzzer, (uint16_t)wrap);
    pwm_set_chan_level(slice_buzzer, channel_buzzer, (uint16_t)(wrap / 2u));
    pwm_set_enabled(slice_buzzer, true);
    sleep_ms(duration_ms);
    buzzer_idle();
}
