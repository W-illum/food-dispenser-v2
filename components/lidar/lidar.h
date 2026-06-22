#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

typedef struct {
    gpio_num_t pwm;
    volatile int64_t rise_us;
    volatile uint16_t pulse_us;
    volatile bool new_sample;
    portMUX_TYPE mux;
} lidar_t;

void lidar_init(lidar_t *l, gpio_num_t pwm);
bool lidar_poll(lidar_t *l, int16_t *mm);