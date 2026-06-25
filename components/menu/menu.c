#include "menu.h"
#include <stdio.h>
#include <stdbool.h>

// ---- dispatch table type ----
typedef struct {
    uint8_t (*option_count)(const menu_t *m);
    void    (*render)(menu_t *m);
    void    (*on_click)(menu_t *m);
    void    (*on_edit_rotate)(menu_t *m, int dir);   // NULL if screen has no EDIT mode
} menu_screen_t;

static const menu_screen_t SCREENS[SCREEN_COUNT]; // defined below the screens

// shared helpers (no dependency on SCREENS)
static void menu_goto(menu_t *m, screen_id_t s)
{
    m->current = s;
    m->selected_index = 1;
    m->mode = MENU_MODE_NAVIGATE;
}

static void menu_emit(menu_t *m, feeder_event_t ev)
{
    if (m->event_queue)
        xQueueSend(m->event_queue, &ev, 0);   // non-blocking
}

// draw "<cursor><text>" at the left edge of a row.
static void draw_item(menu_t *m, uint8_t y, bool selected, const char *text)
{
    char line[24];
    snprintf(line, sizeof(line), "%s%s", selected ? ">" : " ", text);
    ssd1306_draw_text(m->lcd, 0, y, line);
}

// ---- HOME ----
static bool next_feed_time(const menu_t *m, Time_t now, Time_t *next)
{
    uint8_t count = feeding_schedule_size(m->schedule);
    const Time_t *times = feeding_schedule_data(m->schedule);
    if (count == 0) return false;

    for (uint8_t i = 0; i < count; i++)
    {
        if (times[i].hour > now.hour ||
            (times[i].hour == now.hour && times[i].min >= now.min))
        {
            *next = times[i];
            return true;
        }
    }
    *next = times[0]; // none later today, wraps to first tomorrow
    return true;
}

static uint8_t home_options(const menu_t *m) { return 1; }

static void home_render(menu_t *m)
{
    char buf[24];

    snprintf(buf, sizeof(buf), "Time %02d:%02d", m->clock.hour, m->clock.min);
    ssd1306_draw_text(m->lcd, 0, 0, buf);

    Time_t next;
    if (next_feed_time(m, m->clock, &next))
        snprintf(buf, sizeof(buf), "Next %02d:%02d %dg", next.hour, next.min, m->food_amount);
    else
        snprintf(buf, sizeof(buf), "Next --:-- %dg", m->food_amount);
    ssd1306_draw_text(m->lcd, 0, 16, buf);

    if (m->lidar_mm >= 0)
    {
        const int max_mm = 400;
        int clamped = (m->lidar_mm > max_mm) ? max_mm : m->lidar_mm;
        int pct = 100 - (clamped * 100 / max_mm);
        snprintf(buf, sizeof(buf), "Lvl %3dmm %3d%%", m->lidar_mm, pct);
    }
    else if (m->lidar_mm == -2)
    {
        snprintf(buf, sizeof(buf), "Lvl no target");
    }
    else
    {
        snprintf(buf, sizeof(buf), "Lvl reading...");
    }
    ssd1306_draw_text(m->lcd, 0, 32, buf);

    if (m->selected_index == 1)
        ssd1306_draw_text(m->lcd, 0, 48, ">Settings");
}

static void home_click(menu_t *m) { menu_goto(m, SCREEN_SETTINGS); }

// ---- SETTINGS ----
static const char *SETTINGS_ITEMS[] = { "Back", "Schedules", "Food Amount", "System Clock" };

static uint8_t settings_options(const menu_t *m) { return 4; }

static void settings_render(menu_t *m)
{
    for (int i = 0; i < 4; i++)
        draw_item(m, i * 16, (i + 1 == m->selected_index), SETTINGS_ITEMS[i]);
}

static void settings_click(menu_t *m)
{
    switch (m->selected_index)
    {
        case 1:
            menu_goto(m, SCREEN_HOME);
            break;
        case 2:
            menu_goto(m, SCREEN_SCHEDULES);
            break;
        case 3:
            m->edit_food = m->food_amount;
            menu_goto(m, SCREEN_FOOD_AMOUNT);
            break;
        case 4:
            m->edit_time = m->clock;
            menu_goto(m, SCREEN_CLOCK);
            break;
    }
}

// ---- SCHEDULES ----
static const char *SCHED_ITEMS[] = { "Back", "Add", "Remove" };

static uint8_t sched_options(const menu_t *m) { return 3; }

static void sched_render(menu_t *m)
{
    for (int i = 0; i < 3; i++)
        draw_item(m, i * 16, (i + 1 == m->selected_index), SCHED_ITEMS[i]);

    uint8_t count = feeding_schedule_size(m->schedule);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d/%d", count, CONFIG_FEEDING_SCHEDULE_MAX_FEEDS);
    ssd1306_draw_text(m->lcd, 0, 48, buf);

    const Time_t *times = feeding_schedule_data(m->schedule);
    for (int i = 0; i < count; i++)
    {
        snprintf(buf, sizeof(buf), "%02d:%02d", times[i].hour, times[i].min);
        ssd1306_draw_text(m->lcd, 75, i * 16, buf);
    }
}

static void sched_click(menu_t *m)
{
    switch (m->selected_index)
    {
        case 1:
            menu_goto(m, SCREEN_SETTINGS);
            break;
        case 2:
            m->edit_time = (Time_t){0, 0};
            menu_goto(m, SCREEN_ADD);
            break;
        case 3:
            menu_goto(m, SCREEN_REMOVE);
            break;
    }
}

//---- ADD / CLOCK (shared time editor) ----
// index 1 = hour, 2 = minute, 3 = Confirm, 4 = Back
static uint8_t time_editor_options(const menu_t *m) { return 4; }

static void time_editor_render(menu_t *m)
{
    const char *pre = "";
    const char *suf = "";
    if (m->selected_index == 1) pre = (m->mode == MENU_MODE_EDIT) ? ">*" : ">";
    if (m->selected_index == 2) suf = (m->mode == MENU_MODE_EDIT) ? "*<" : "<";

    char line[16];
    snprintf(line, sizeof(line), "%s%02d:%02d%s", pre, m->edit_time.hour, m->edit_time.min, suf);
    ssd1306_draw_text(m->lcd, 0, 0, line);

    draw_item(m, 24, (m->selected_index == 3), "Confirm");
    draw_item(m, 48, (m->selected_index == 4), "Back");
}

static void wrap_hour(Time_t *t, int dir)
{
    int h = t->hour + dir;
    if (h < 0)  h = 23;
    if (h > 23) h = 0;
    t->hour = (uint8_t)h;
}

static void add_edit_rotate(menu_t *m, int dir)   // minute step of 5
{
    if (m->selected_index == 1)
    {
        wrap_hour(&m->edit_time, dir);
    }
    else if (m->selected_index == 2)
    {
        int min = m->edit_time.min + dir * 5;
        if (min < 0)  min = 55;
        if (min > 55) min = 0;
        m->edit_time.min = (uint8_t)min;
    }
}

static void clock_edit_rotate(menu_t *m, int dir) // minute step of 1
{
    if (m->selected_index == 1)
    {
        wrap_hour(&m->edit_time, dir);
    }
    else if (m->selected_index == 2)
    {
        int min = m->edit_time.min + dir;
        if (min < 0)  min = 59;
        if (min > 59) min = 0;
        m->edit_time.min = (uint8_t)min;
    }
}

// click on a value field toggles EDIT; shared by ADD and CLOCK.
static bool time_editor_toggle_edit(menu_t *m)
{
    if (m->selected_index == 1 || m->selected_index == 2)
    {
        m->mode = (m->mode == MENU_MODE_NAVIGATE) ? MENU_MODE_EDIT : MENU_MODE_NAVIGATE;
        return true;
    }
    return false;
}

static void add_click(menu_t *m)
{
    if (time_editor_toggle_edit(m)) return;
    switch (m->selected_index)
    {
        case 3:
            menu_emit(m,
                (feeder_event_t){
                    .type = FEEDER_EVENT_TIME_ADDED,
                    .time = m->edit_time
                }
            );
            menu_goto(m, SCREEN_SCHEDULES);
            break;
        case 4:
            menu_goto(m, SCREEN_SCHEDULES);
            break;
    }
}

static void clock_click(menu_t *m)
{
    if (time_editor_toggle_edit(m)) return;
    switch (m->selected_index) {
        case 3:
            menu_emit(m,
                (feeder_event_t){
                    .type = FEEDER_EVENT_CLOCK_UPDATED,
                    .time = m->edit_time
                }
            );
            menu_goto(m, SCREEN_SETTINGS);
            break;
        case 4:
            menu_goto(m, SCREEN_SETTINGS);
            break;
    }
}

// ---- FOOD_AMOUNT ----
// index 1 = amount field, 2 = Confirm
static uint8_t food_options(const menu_t *m) { return 2; }

static void food_render(menu_t *m)
{
    const char *cur = (m->selected_index == 1) ? ">" : " ";
    const char *ed  = (m->mode == MENU_MODE_EDIT) ? "*" : " ";

    char line[16];
    snprintf(line, sizeof(line), "%s%s%d g", cur, ed, m->edit_food);
    ssd1306_draw_text(m->lcd, 0, 0, line);

    draw_item(m, 24, (m->selected_index == 2), "Confirm");
}

static void food_edit_rotate(menu_t *m, int dir)
{
    m->edit_food += dir * 5;
    if (m->edit_food < 0)  m->edit_food = 0;
    if (m->edit_food > 50) m->edit_food = 50;
}

static void food_click(menu_t *m)
{
    switch (m->selected_index)
    {
        case 1:
            m->mode = (m->mode == MENU_MODE_NAVIGATE) ? MENU_MODE_EDIT : MENU_MODE_NAVIGATE;
            break;
        case 2:
            menu_emit(m,
                (feeder_event_t){
                    .type  = FEEDER_EVENT_FOOD_UPDATED,
                    .grams = (uint8_t)m->edit_food
                }
            );
            menu_goto(m, SCREEN_SETTINGS);
            break;
    }
}

// ---- REMOVE ----
//index 1 = Back, 2..(1+count) = remove that scheduled time
static uint8_t remove_options(const menu_t *m)
{
    return 1 + feeding_schedule_size(m->schedule);
}

static void remove_render(menu_t *m)
{
    draw_item(m, 0, (m->selected_index == 1), "Back");

    uint8_t count = feeding_schedule_size(m->schedule);
    const Time_t *times = feeding_schedule_data(m->schedule);
    char buf[8];
    for (int i = 0; i < count; i++)
    {
        if (m->selected_index == i + 2)
            ssd1306_draw_text(m->lcd, 65, i * 16, ">");
        snprintf(buf, sizeof(buf), "%02d:%02d", times[i].hour, times[i].min);
        ssd1306_draw_text(m->lcd, 75, i * 16, buf);
    }
}

static void remove_click(menu_t *m)
{
    if (m->selected_index == 1)
    {
        menu_goto(m, SCREEN_SCHEDULES);
        return;
    }
    uint8_t idx = (uint8_t)(m->selected_index - 2);
    menu_emit(m,
        (feeder_event_t){
            .type = FEEDER_EVENT_TIME_REMOVED,
            .index = idx
        }
    );
    m->selected_index = 1;     // stay on screen so multiple removals are easy
}

// ---- dispatch table ----
static const menu_screen_t SCREENS[SCREEN_COUNT] = {
//  SCREENS[index]           (*option count)      (*render)           (*on_click)     (*on_edit_rotate)
    [SCREEN_HOME]        = { home_options,        home_render,        home_click,     NULL              },
    [SCREEN_SETTINGS]    = { settings_options,    settings_render,    settings_click, NULL              },
    [SCREEN_SCHEDULES]   = { sched_options,       sched_render,       sched_click,    NULL              },
    [SCREEN_ADD]         = { time_editor_options, time_editor_render, add_click,      add_edit_rotate   },
    [SCREEN_FOOD_AMOUNT] = { food_options,        food_render,        food_click,     food_edit_rotate  },
    [SCREEN_REMOVE]      = { remove_options,      remove_render,      remove_click,   NULL              },
    [SCREEN_CLOCK]       = { time_editor_options, time_editor_render, clock_click,    clock_edit_rotate },
};

// ---- generic navigation (uses the table) ----
static void menu_navigate(menu_t *m, int dir)
{
    uint8_t max = SCREENS[m->current].option_count(m);
    if (max == 0) return;
    m->selected_index += dir;
    if (m->selected_index > (int)max) m->selected_index = 1;
    if (m->selected_index < 1)        m->selected_index = max;
}

// ---- public API ----
void menu_init(menu_t *m, ssd1306_t *lcd, const feeding_schedule_t *schedule,
    QueueHandle_t event_queue, uint8_t food_amount, Time_t clock)
{
    m->lcd = lcd;
    m->schedule = schedule;
    m->event_queue = event_queue;
    m->current = SCREEN_HOME;
    m->mode = MENU_MODE_NAVIGATE;
    m->selected_index = 1;
    m->food_amount = food_amount;
    m->lidar_mm = -1;
    m->clock = clock;
    m->edit_time = (Time_t){0, 0};
    m->edit_food = food_amount;
}

void menu_on_rotate(menu_t *m, int dir)
{
    if (m->mode == MENU_MODE_EDIT && SCREENS[m->current].on_edit_rotate)
        SCREENS[m->current].on_edit_rotate(m, dir);
    else
        menu_navigate(m, dir);
}

void menu_on_click(menu_t *m)
{
    if (SCREENS[m->current].on_click)
        SCREENS[m->current].on_click(m);
}

void menu_render(menu_t *m)
{
    ssd1306_clear(m->lcd);
    if (SCREENS[m->current].render)
        SCREENS[m->current].render(m);
    ssd1306_flush(m->lcd);
}

void menu_set_food_amount(menu_t *m, uint8_t grams) { m->food_amount = grams; }
void menu_set_lidar_mm(menu_t *m, int16_t mm)       { m->lidar_mm = mm; }
void menu_set_clock(menu_t *m, Time_t clock)        { m->clock = clock; }
