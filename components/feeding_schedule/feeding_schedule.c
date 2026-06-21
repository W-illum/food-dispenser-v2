#include "feeding_schedule.h"

void feeding_schedule_init(feeding_schedule_t *s)
{
    for (int i = 0; i < CONFIG_FEEDING_SCHEDULE_MAX_FEEDS; i++)
        s->times[i] = (Time_t){99, 99};
    s->count = 0;
}

uint8_t feeding_schedule_size(const feeding_schedule_t *s)
{
    return s->count;
}

const Time_t *feeding_schedule_data(const feeding_schedule_t *s)
{
    return s->times;
}

bool feeding_schedule_add(feeding_schedule_t *s, Time_t time)
{
    if (s->count >= CONFIG_FEEDING_SCHEDULE_MAX_FEEDS) return false;
    if (feeding_schedule_contains(s, time)) return false;

    int i = s->count - 1;
    while (i >= 0 && is_time_after(s->times[i], time))
    {
        s->times[i + 1] = s->times[i];
        i--;
    }
    s->times[i + 1] = time;
    s->count++;
    return true;
}

void feeding_schedule_remove(feeding_schedule_t *s, uint8_t index)
{
    if (index >= s->count) return;

    for (int i = index; i < s->count - 1; i++)
    {
        s->times[i] = s->times[i + 1];
    }
    s->count--;
    s->times[s->count] = (Time_t){99, 99};
}

bool feeding_schedule_contains(const feeding_schedule_t *s, Time_t time)
{
    for (int i = 0; i < s->count; i++)
    {
        if (s->times[i].hour == time.hour && s->times[i].min == time.min) return true;
    }
    return false;
}
