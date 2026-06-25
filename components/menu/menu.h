#pragma once
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "ssd1306.h"
#include "feeding_schedule.h"
#include "feeder_time.h"
#include "feeder_event.h"

typedef enum {
    SCREEN_HOME = 0,
    SCREEN_SETTINGS,
    SCREEN_SCHEDULES,
    SCREEN_ADD,
    SCREEN_FOOD_AMOUNT,
    SCREEN_REMOVE,
    SCREEN_CLOCK,
    SCREEN_COUNT          // sizes the dispatch table automatically
} screen_id_t;

typedef enum {
    MENU_MODE_NAVIGATE = 0,
    MENU_MODE_EDIT,
} menu_mode_t;

typedef struct {
    ssd1306_t                *lcd;          // injected dependencies
    const feeding_schedule_t *schedule;
    QueueHandle_t             event_queue;  // intents go out here

    screen_id_t current;
    menu_mode_t mode;
    int         selected_index;

    uint8_t food_amount;                    // values shown on HOME
    int16_t lidar_mm;
    Time_t  clock;

    Time_t  edit_time;                      // ADD / CLOCK scratch (mutually exclusive)
    int     edit_food;                      // FOOD_AMOUNT scratch
} menu_t;

void menu_init(menu_t *m, ssd1306_t *lcd, const feeding_schedule_t *schedule,
    QueueHandle_t event_queue, uint8_t food_amount, Time_t clock);

void menu_on_rotate(menu_t *m, int dir);    // +1 = CW, -1 = CCW
void menu_on_click(menu_t *m);
void menu_render(menu_t *m);

// the feeder pushes fresh display values in:
void menu_set_food_amount(menu_t *m, uint8_t grams);
void menu_set_lidar_mm(menu_t *m, int16_t mm);
void menu_set_clock(menu_t *m, Time_t clock);