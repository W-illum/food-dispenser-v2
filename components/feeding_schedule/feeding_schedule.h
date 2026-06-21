#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "feeder_time.h"

typedef struct {
    Time_t times[CONFIG_FEEDING_SCHEDULE_MAX_FEEDS];
    uint8_t count;
} feeding_schedule_t;

void feeding_schedule_init(feeding_schedule_t *s);
uint8_t feeding_schedule_size(const feeding_schedule_t* s);
const Time_t *feeding_schedule_data(const feeding_schedule_t *s);
bool feeding_schedule_add(feeding_schedule_t *s, Time_t time);
void feeding_schedule_remove(feeding_schedule_t *s, uint8_t index);
bool feeding_schedule_contains(const feeding_schedule_t *s, Time_t time);
