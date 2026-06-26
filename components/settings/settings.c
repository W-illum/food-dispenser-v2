#include "settings.h"
#include "nvs_flash.h"
#include "nvs.h"

#define NS               "feeder"
#define KEY_VERSION      "version"
#define KEY_FOOD         "food"
#define KEY_SCHED        "sched"
#define SETTINGS_VERSION 1 // bump when settings layout changes

esp_err_t settings_init(void)
{
    // mounts the nvs flash partition (found in partitions.csv)
    esp_err_t  err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t settings_load(uint8_t *food_amount, feeding_schedule_t *schedule)
{
    nvs_handle_t h;

    // open nvs as readonly, fails when no data is written (ex. first boot)
    esp_err_t err = nvs_open(NS, NVS_READONLY, &h);
    if (err != ESP_OK)
        return err;

    // ignore data written by a firmware with a different settings layout.
    uint8_t version = 0;
    err = nvs_get_u8(h, KEY_VERSION, &version);
    if (err != ESP_OK || version != SETTINGS_VERSION)
    {
        nvs_close(h);
        return ESP_ERR_NVS_NOT_FOUND;
    }

    // fetch stored food value (uint8_t)
    uint8_t tmp_food;
    err = nvs_get_u8(h, KEY_FOOD, &tmp_food);
    if (err != ESP_OK)
    {
        nvs_close(h);
        return err;
    }

    // fetch stored schedule data (blob data)
    feeding_schedule_t tmp_sched;
    size_t size = sizeof(tmp_sched);
    err = nvs_get_blob(h, KEY_SCHED, &tmp_sched, &size);
    nvs_close(h);

    if (err != ESP_OK)
        return err;
    if (size != sizeof(tmp_sched))
        return ESP_ERR_INVALID_SIZE;
    if (tmp_sched.count > CONFIG_FEEDING_SCHEDULE_MAX_FEEDS)
        return ESP_ERR_INVALID_STATE;

    // commit the fetched data when everything's valid
    *schedule = tmp_sched;
    *food_amount = tmp_food;

    return ESP_OK;
}

esp_err_t settings_save(uint8_t food_amount, const feeding_schedule_t *schedule)
{
    nvs_handle_t h;

    // open nvs as readwrite, creates the namespace if non existent
    esp_err_t err = nvs_open(NS, NVS_READWRITE, &h);
    if (err != ESP_OK)
        return err;

    // stage the writes
    err = nvs_set_u8(h, KEY_VERSION, SETTINGS_VERSION);
    if (err == ESP_OK)
        err = nvs_set_u8(h, KEY_FOOD, food_amount);
    if (err == ESP_OK)
        err = nvs_set_blob(h, KEY_SCHED, schedule, sizeof(*schedule));
    if (err == ESP_OK)
        err = nvs_commit(h); // settings saved

    nvs_close(h);
    return err;
}