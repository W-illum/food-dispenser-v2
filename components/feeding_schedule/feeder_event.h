#pragma once
#include <stdint.h>
#include "feeder_time.h"

/* Intent emitted by any input source (menu, BLE) and consumed by the feeder.
 * Sent through a FreeRTOS queue where the union holds only the payload that the
 * event type actually needs. */

typedef enum {
    FEEDER_EVENT_NONE = 0,
    FEEDER_EVENT_TIME_ADDED,     // .time  - add this feeding time
    FEEDER_EVENT_TIME_REMOVED,   // .index - remove schedule entry at this index
    FEEDER_EVENT_FOOD_UPDATED,   // .grams - new food amount
    FEEDER_EVENT_CLOCK_UPDATED,  // .time  - new system clock (hour/min)
    FEEDER_EVENT_MANUAL_FEED,    // .grams - dispense now (0 = use stored amount)
} feeder_event_type_t;

typedef struct {
    feeder_event_type_t type;
    union {
        Time_t  time;
        uint8_t index;
        uint8_t grams;
    };
} feeder_event_t;