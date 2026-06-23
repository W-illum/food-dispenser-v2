#include "ds1302.h"
#include "esp_rom_sys.h"

#define REG_SECONDS 0x80
#define REG_WP      0x8E
#define REG_BURST   0xBE

static void ds1302_set_direction(ds1302_t *ds, gpio_mode_t dir)
{
    if (ds->dat_direction == dir) return;

    ds->dat_direction = dir;
    gpio_set_direction(ds->dat, dir);
}

static void ds1302_next_bit(ds1302_t *ds)
{
    gpio_set_level(ds->clk, 1);
    esp_rom_delay_us(1);

    gpio_set_level(ds->clk, 0);
    esp_rom_delay_us(1);
}

static void ds1302_write_byte(ds1302_t *ds, uint8_t value)
{
    for(uint8_t b = 0; b < 8; b++)
    {
        gpio_set_level(ds->dat, (value & 0x01) ? 1 : 0);
        ds1302_next_bit(ds);
        value >>= 1;
    }
}

static uint8_t ds1302_read_byte(ds1302_t *ds)
{
    uint8_t byte = 0x00;

    for(uint8_t b = 0; b < 8; b++)
    {
        if (gpio_get_level(ds->dat) == 1) byte |= 0x01 << b;
        ds1302_next_bit(ds);
    }
    return byte;
}

static void ds1302_prepare_read(ds1302_t *ds, uint8_t addr)
{
    ds1302_set_direction(ds, GPIO_MODE_OUTPUT);
    gpio_set_level(ds->en, 1);
    uint8_t cmd = 0b10000001 | addr;
    ds1302_write_byte(ds, cmd);
    ds1302_set_direction(ds, GPIO_MODE_INPUT);
}

static void ds1302_prepare_write(ds1302_t *ds, uint8_t addr)
{
    ds1302_set_direction(ds, GPIO_MODE_OUTPUT);
    gpio_set_level(ds->en, 1);
    uint8_t cmd = 0b10000000 | addr;
    ds1302_write_byte(ds, cmd);
}

static void ds1302_end(ds1302_t *ds)
{
    gpio_set_level(ds->en, 0);
}

static uint8_t ds1302_dec_to_bcd(uint8_t dec)
{
    return ((dec / 10 * 16) + (dec % 10));
}

static uint8_t ds1302_bcd_to_dec(uint8_t bcd)
{
    return ((bcd / 16 * 10) + (bcd % 16));
}

void ds1302_init(ds1302_t *ds, gpio_num_t en, gpio_num_t clk, gpio_num_t dat)
{
    ds->en  = en;
    ds->clk = clk;
    ds->dat = dat;
    ds->dat_direction = GPIO_MODE_INPUT;

    gpio_config_t io = {
        .pin_bit_mask = (1ULL << en) | (1ULL << clk) | (1ULL << dat),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_direction(dat, ds->dat_direction);

    gpio_set_level(en, 0);
    gpio_set_level(clk, 0);
}

bool ds1302_is_halted(ds1302_t *ds)
{
    ds1302_prepare_read(ds, REG_SECONDS);
    uint8_t seconds = ds1302_read_byte(ds);
    ds1302_end(ds);
    return (seconds & 0b10000000);
}

void ds1302_get_date_time(ds1302_t *ds, date_time_t *dt)
{
    ds1302_prepare_read(ds, REG_BURST);
    dt->second = ds1302_bcd_to_dec(ds1302_read_byte(ds) & 0b01111111);
    dt->minute = ds1302_bcd_to_dec(ds1302_read_byte(ds) & 0b01111111);
    dt->hour   = ds1302_bcd_to_dec(ds1302_read_byte(ds) & 0b00111111);
    dt->day    = ds1302_bcd_to_dec(ds1302_read_byte(ds) & 0b00111111);
    dt->month  = ds1302_bcd_to_dec(ds1302_read_byte(ds) & 0b00011111);
    dt->dow    = ds1302_bcd_to_dec(ds1302_read_byte(ds) & 0b00000111);
    dt->year   = ds1302_bcd_to_dec(ds1302_read_byte(ds) & 0b01111111);
    ds1302_end(ds);
}

void ds1302_set_date_time(ds1302_t *ds, const date_time_t *dt)
{
    ds1302_prepare_write(ds, REG_WP);
    ds1302_write_byte(ds, 0b00000000);
    ds1302_end(ds);

    ds1302_prepare_write(ds, REG_BURST);
    ds1302_write_byte(ds, ds1302_dec_to_bcd(dt->second % 60));
    ds1302_write_byte(ds, ds1302_dec_to_bcd(dt->minute % 60));
    ds1302_write_byte(ds, ds1302_dec_to_bcd(dt->hour   % 24));
    ds1302_write_byte(ds, ds1302_dec_to_bcd(dt->day    % 32));
    ds1302_write_byte(ds, ds1302_dec_to_bcd(dt->month  % 13));
    ds1302_write_byte(ds, ds1302_dec_to_bcd(dt->dow    % 8));
    ds1302_write_byte(ds, ds1302_dec_to_bcd(dt->year   % 100));
    ds1302_write_byte(ds, 0b10000000);
    ds1302_end(ds);
}