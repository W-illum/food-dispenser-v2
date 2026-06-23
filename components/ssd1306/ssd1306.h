#pragma once
#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c_master.h"

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_PAGES   (SSD1306_HEIGHT / 8)

typedef struct {
    i2c_master_dev_handle_t dev;
    uint8_t buffer[SSD1306_WIDTH * SSD1306_PAGES];
} ssd1306_t;

esp_err_t ssd1306_init(ssd1306_t *d, i2c_master_bus_handle_t bus, uint8_t addr);
void ssd1306_clear(ssd1306_t *d);
void ssd1306_draw_text(ssd1306_t *d, uint8_t x, uint8_t y, const char *str);
esp_err_t ssd1306_flush(ssd1306_t *d);
