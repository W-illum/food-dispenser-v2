#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "feeding_schedule.h"

esp_err_t settings_init(void);
esp_err_t settings_load(uint8_t *food_amount, feeding_schedule_t *schedule);
esp_err_t settings_save(uint8_t food_amount, const feeding_schedule_t *schedule);