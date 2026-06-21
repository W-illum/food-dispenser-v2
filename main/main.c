#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BLINK_GPIO 48
#define BLINK_MS 1000

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BLINK_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    int level = 0;
    while (true)
    {
        gpio_set_level(BLINK_GPIO, level);
        level ^= 1;
        vTaskDelay(pdMS_TO_TICKS(BLINK_MS));
    }
}
