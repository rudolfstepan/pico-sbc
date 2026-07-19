#include "touch_gt911.h"

#include "board.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"

#include <stddef.h>

#define TOUCH_I2C i2c0
#define TOUCH_BAUD 100000
#define GT911_ADDR_A 0x5d
#define GT911_ADDR_B 0x14
#define REG_STATUS 0x814e
#define REG_POINT1 0x8150

static uint8_t gt911_addr = GT911_ADDR_A;
static bool initialized;

static bool gt911_write_reg(uint16_t reg, const uint8_t *data, size_t len) {
    uint8_t buf[10];
    if (len + 2 > sizeof buf) return false;
    buf[0] = (uint8_t)(reg >> 8);
    buf[1] = (uint8_t)reg;
    for (size_t i = 0; i < len; ++i) buf[i + 2] = data[i];
    return i2c_write_blocking(TOUCH_I2C, gt911_addr, buf, len + 2, false) >= 0;
}

static bool gt911_read_reg(uint16_t reg, uint8_t *data, size_t len) {
    uint8_t addr_buf[2] = {(uint8_t)(reg >> 8), (uint8_t)reg};
    if (i2c_write_blocking(TOUCH_I2C, gt911_addr, addr_buf, 2, true) < 0) {
        return false;
    }
    return i2c_read_blocking(TOUCH_I2C, gt911_addr, data, len, false) >= 0;
}

static bool gt911_probe(uint8_t addr) {
    gt911_addr = addr;
    uint8_t product_id[4];
    return gt911_read_reg(0x8140, product_id, sizeof product_id);
}

static void gt911_reset(bool address_14) {
    gpio_set_function(PIN_TOUCH_RST, GPIO_FUNC_SIO);
    gpio_set_function(PIN_TOUCH_INT, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_TOUCH_RST, GPIO_OUT);
    gpio_set_dir(PIN_TOUCH_INT, GPIO_OUT);

    gpio_put(PIN_TOUCH_RST, 0);
    gpio_put(PIN_TOUCH_INT, address_14 ? 1 : 0);
    sleep_ms(20);
    gpio_put(PIN_TOUCH_RST, 1);
    sleep_ms(6);

    /* The GT911 needs this sync pulse after reset or it can stay asleep. */
    gpio_put(PIN_TOUCH_INT, 0);
    sleep_ms(50);
    gpio_set_dir(PIN_TOUCH_INT, GPIO_IN);
    gpio_pull_up(PIN_TOUCH_INT);
    sleep_ms(100);
}

bool touch_init(void) {
    i2c_init(TOUCH_I2C, TOUCH_BAUD);
    gpio_set_function(PIN_TOUCH_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_TOUCH_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_TOUCH_SDA);
    gpio_pull_up(PIN_TOUCH_SCL);

    gpio_init(PIN_TOUCH_RST);
    gpio_init(PIN_TOUCH_INT);

    gt911_reset(false);
    initialized = gt911_probe(GT911_ADDR_A);
    if (!initialized) {
        gt911_reset(true);
        initialized = gt911_probe(GT911_ADDR_B);
    }

    if (initialized) {
        uint8_t clear = 0;
        (void)gt911_write_reg(REG_STATUS, &clear, 1);
    }
    return initialized;
}

bool touch_is_ready(void) {
    return initialized;
}

bool touch_read(touch_point_t *point) {
    uint8_t status = 0;
    point->touched = false;
    point->updated = false;
    if (!initialized) {
        return false;
    }
    if (!gt911_read_reg(REG_STATUS, &status, 1)) {
        return false;
    }
    if ((status & 0x80u) == 0) {
        return true;
    }

    point->updated = true;

    uint8_t count = status & 0x0fu;
    if (count > 0) {
        uint8_t data[8];
        if (gt911_read_reg(REG_POINT1, data, sizeof data)) {
            uint16_t raw_x = (uint16_t)data[1] << 8 | data[0];
            uint16_t raw_y = (uint16_t)data[3] << 8 | data[2];

            if (raw_x < 320 && raw_y < 480) {
                point->x = raw_y;
                point->y = 319u - raw_x;
                point->touched = true;
            }
        }
    }

    uint8_t clear = 0;
    (void)gt911_write_reg(REG_STATUS, &clear, 1);
    return true;
}
