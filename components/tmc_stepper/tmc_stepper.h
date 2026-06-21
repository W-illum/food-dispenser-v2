#pragma once
#include <stdint.h>
#include "driver/gpio.h"
#include "driver/gptimer.h"

typedef enum {
    TMC_DIR_CCW = 0,
    TMC_DIR_CW  = 1,
} tmc_dir_t;

typedef struct {
    gpio_num_t dir, step, ms2, ms1, en;
    int micro_step;
    int steps_per_rev;
    volatile uint32_t remaining_steps;
    gptimer_handle_t timer;
} tmc_stepper_t;

void tmc_stepper_init(tmc_stepper_t *s, gpio_num_t dir, gpio_num_t step,
    gpio_num_t ms2, gpio_num_t ms1, gpio_num_t en);
void tmc_stepper_set_micro_step(tmc_stepper_t *s, int step);
void tmc_stepper_rotate(tmc_stepper_t *s, unsigned int deg, tmc_dir_t dir);
