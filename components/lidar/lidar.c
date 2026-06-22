#include "lidar.h"
#include "esp_timer.h"
#include "esp_attr.h"

#define LIDAR_NO_TARGET_PULSE_US 1850

static int16_t lidar_pulse_to_mm(uint16_t pulse_us)
{
    if (pulse_us == 0) return -1;
    if (pulse_us > LIDAR_NO_TARGET_PULSE_US) return -2;

    int16_t d = ((int16_t)pulse_us - 1000) * 3 / 4;
    if (d < 0) d = 0;
    return d;
}

static void IRAM_ATTR lidar_isr(void *arg)
{
    lidar_t *l = (lidar_t *)arg;

    int64_t now = esp_timer_get_time();
    int level = gpio_get_level(l->pwm);

    portENTER_CRITICAL_ISR(&l->mux);
    if (level == 1)
    {
        l->rise_us = now;
    }
    else if (l->rise_us != 0)
    {
        l->pulse_us = (uint16_t)(now - l->rise_us);
        l->new_sample = true;
    }
    portEXIT_CRITICAL_ISR(&l->mux);
}

void lidar_init(lidar_t *l, gpio_num_t pwm)
{
    l->pwm = pwm;
    l->rise_us = 0;
    l->pulse_us = 0;
    l->new_sample = false;
    l->mux = (portMUX_TYPE)portMUX_INITIALIZER_UNLOCKED;

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << pwm),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io);

    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(err);
    }

    gpio_isr_handler_add(pwm, lidar_isr, l);
}

bool lidar_poll(lidar_t *l, int16_t *mm)
{
    if (!l->new_sample) return false;

    portENTER_CRITICAL(&l->mux);
    uint16_t pulse = l->pulse_us;
    l->new_sample = false;
    portEXIT_CRITICAL_ISR(&l->mux);

    int16_t d = lidar_pulse_to_mm(pulse);
    if (d == -1) return false;
    *mm = d;
    return true;
}