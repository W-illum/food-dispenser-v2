#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t hour;
    uint8_t min;
} Time_t;

// returns true if t1 is later in the day then t2
static inline bool is_time_after(Time_t t1, Time_t t2)
{
    if (t1.hour != t2.hour) return t1.hour > t2.hour;
    return t1.min > t2.min;
}