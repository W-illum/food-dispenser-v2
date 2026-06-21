#pragma once
#include <stdint.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum{
    ENCODER_EVENT_NONE = 0,
    ENCODER_EVENT_CW,
    ENCODER_EVENT_CCW,
    ENCODER_EVENT_BTN,
} encoder_event_t;

typedef struct {
    gpio_num_t sw, a, b;
    int current_state;
    int64_t last_btn_us;
    QueueHandle_t queue;
} rotary_encoder_t;

void rotary_encoder_init(rotary_encoder_t *e, gpio_num_t sw,
    gpio_num_t a, gpio_num_t b);

encoder_event_t rotary_encoder_check(rotary_encoder_t *e);