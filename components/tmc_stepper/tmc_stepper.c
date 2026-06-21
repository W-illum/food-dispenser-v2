#include "tmc_stepper.h"
#include "esp_rom_sys.h"
#include "esp_attr.h"

static bool IRAM_ATTR on_step_timer(gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata, void *user_data);

void tmc_stepper_init(tmc_stepper_t *s, gpio_num_t dir, gpio_num_t step,
    gpio_num_t ms2, gpio_num_t ms1, gpio_num_t en)
{
    s->dir  = dir;
    s->step = step;
    s->ms2  = ms2;
    s->ms1  = ms1;
    s->en   = en;

    s->steps_per_rev = 200;
    s->remaining_steps = 0;
    s->timer = NULL;

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << dir) | (1ULL << step) | (1ULL << ms2) |
                        (1ULL << ms1) | (1ULL << en),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);

    gpio_set_level(en, 1);
    tmc_stepper_set_micro_step(s, 8);

    // Hardware timer
    gptimer_config_t timer_cfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1Mhz
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_cfg, &s->timer));

    // Alarm callback
    gptimer_event_callbacks_t cbs = {
        .on_alarm = on_step_timer,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(s->timer, &cbs, s));

    // Alarm action
    gptimer_alarm_config_t alarm_cfg = {
        .alarm_count = 1000,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(s->timer, &alarm_cfg));

    // Enable alarm (not started)
    ESP_ERROR_CHECK(gptimer_enable(s->timer));
}

void tmc_stepper_set_micro_step(tmc_stepper_t *s, int step)
{
    /* Stated in datasheet
    MS1     MS2     STEPS
    GND     GND     8
    VIO     GND     2
    GND     VIO     4
    VIO     VIO     16
    */
    s->micro_step = step;

    switch (step)
    {
    case 8:
        gpio_set_level(s->ms1, 0);
        gpio_set_level(s->ms2, 0);
        break;

    case 2:
        gpio_set_level(s->ms1, 1);
        gpio_set_level(s->ms2, 0);
        break;

    case 4:
        gpio_set_level(s->ms1, 0);
        gpio_set_level(s->ms2, 1);
        break;
    case 16:
        gpio_set_level(s->ms1, 1);
        gpio_set_level(s->ms2, 1);
        break;

    default:
        break;
    }
}

void tmc_stepper_rotate(tmc_stepper_t *s, unsigned int deg, tmc_dir_t dir)
{
    gpio_set_level(s->en, 0);
    gpio_set_level(s->dir, dir == TMC_DIR_CW ? 1 : 0);

    bool was_idle = (s->remaining_steps == 0);
    s->remaining_steps = (uint32_t)(deg * s->steps_per_rev * s->micro_step / 360.0);

    if (was_idle)
    {
        gptimer_set_raw_count(s->timer, 0);
        ESP_ERROR_CHECK(gptimer_start(s->timer));
    }
}

static bool on_step_timer(gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata, void *user_data)
{
    tmc_stepper_t *s = (tmc_stepper_t *)user_data;
    if (s->remaining_steps > 0)
    {
        gpio_set_level(s->step, 1);
        esp_rom_delay_us(2);
        gpio_set_level(s->step, 0);
        s->remaining_steps--;
    }
    else
    {
        gptimer_stop(timer);
        gpio_set_level(s->en, 1);
    }

    return false;
}