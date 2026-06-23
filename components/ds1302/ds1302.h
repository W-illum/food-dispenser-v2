#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

typedef struct {
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t dow;
} date_time_t;

typedef struct {
    gpio_num_t en;
    gpio_num_t clk;
    gpio_num_t dat;
    gpio_mode_t dat_direction;
} ds1302_t;

void ds1302_init(ds1302_t *ds, gpio_num_t en, gpio_num_t clk, gpio_num_t dat);
bool ds1302_is_halted(ds1302_t *ds);
void ds1302_get_date_time(ds1302_t *ds, date_time_t *dt);
void ds1302_set_date_time(ds1302_t *ds, const date_time_t *dt);