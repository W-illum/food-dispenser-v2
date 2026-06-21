#include "rotary_encoder.h"
#include "esp_timer.h"

#define BTN_DEBOUNCE_MS 350000

typedef enum {
    STATE_IDLE = 0,
    STATE_CW1, STATE_CW2, STATE_CW3,
    STATE_CCW1, STATE_CCW2, STATE_CCW3,
} rotation_state_t;

static void encoder_isr(void *arg)
{
    rotary_encoder_t *e = (rotary_encoder_t *)arg;

    int sw = gpio_get_level(e->sw);
    int a  = gpio_get_level(e->a);
    int b  = gpio_get_level(e->b);

    BaseType_t hpw = pdFALSE;

    if (sw == 0)
    {
        int64_t now = esp_timer_get_time();
        if (now - e->last_btn_us < BTN_DEBOUNCE_MS) return;
        e->last_btn_us = now;

        encoder_event_t ev = ENCODER_EVENT_BTN;
        xQueueSendFromISR(e->queue, &ev, &hpw);
    }
    else
    {
        switch (e->current_state)
        {
            case STATE_IDLE:
                if      (a == 0 && b == 1) e->current_state = STATE_CW1;
                else if (a == 1 && b == 0) e->current_state = STATE_CCW1;
                break;

            case STATE_CW1:
                if      (a == 0 && b == 0) e->current_state = STATE_CW2;
                else if (a == 1 && b == 0) e->current_state = STATE_IDLE;
                break;

            case STATE_CW2:
                if      (a == 1 && b == 0) e->current_state = STATE_CW3;
                else if (a == 1 && b == 1) e->current_state = STATE_IDLE;
                break;

            case STATE_CW3:
                if (a == 1 && b == 1)
                {
                    encoder_event_t ev = ENCODER_EVENT_CW;
                    xQueueSendFromISR(e->queue, &ev, &hpw);
                    e->current_state = STATE_IDLE;
                }
                break;

            case STATE_CCW1:
                if      (a == 0 && b == 0) e->current_state = STATE_CCW2;
                else if (a == 1 && b == 1) e->current_state = STATE_IDLE;
                break;

            case STATE_CCW2:
                if      (a == 0 && b == 1) e->current_state = STATE_CCW3;
                else if (a == 1 && b == 1) e->current_state = STATE_IDLE;
                break;

            case STATE_CCW3:
                if (a == 1 && b == 1)
                {
                    encoder_event_t ev = ENCODER_EVENT_CCW;
                    xQueueSendFromISR(e->queue, &ev, &hpw);
                    e->current_state = STATE_IDLE;
                }
                break;

            default: break;
        }
    }

    if (hpw == pdTRUE) portYIELD_FROM_ISR();
}

void rotary_encoder_init(rotary_encoder_t *e, gpio_num_t sw,
    gpio_num_t a, gpio_num_t b)
{
    e->sw = sw;
    e->a  = a;
    e->b  = b;
    e->current_state = STATE_IDLE;
    e->last_btn_us = 0;

    e->queue = xQueueCreate(8, sizeof(encoder_event_t));

    // A and B: interrupt on any edge. (SW overridden below)
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << sw) | (1ULL << a) | (1ULL << b),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io);
    gpio_set_intr_type(sw, GPIO_INTR_NEGEDGE); // falling edge only on button

    // Install the shared GPIO ISR service ONCE for the whole app
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_ERROR_CHECK(err);
    }

    gpio_isr_handler_add(sw, encoder_isr, e);
    gpio_isr_handler_add(a,  encoder_isr, e);
    gpio_isr_handler_add(b,  encoder_isr, e);
}

encoder_event_t rotary_encoder_check(rotary_encoder_t *e)
{
    encoder_event_t ev;
    if (xQueueReceive(e->queue, &ev, 0) == pdTRUE)
    {
        return ev;
    }
    return ENCODER_EVENT_NONE;
}
