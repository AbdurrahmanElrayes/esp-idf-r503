/*
 * Copyright 2026 Abdelrahman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "esp_check.h"
#include "esp_log.h"

#include "r503_defs.h"
#include "r503_errors.h"
#include "r503_priv.h"

static const char *TAG = "r503_led";

esp_err_t r503_aura_led_config(r503_t *dev,
                               r503_led_mode_t mode,
                               uint8_t speed,
                               r503_led_color_t color,
                               uint8_t times)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");

    ESP_RETURN_ON_FALSE(
        mode >= R503_LED_BREATHING && mode <= R503_LED_GRADUAL_OFF,
        ESP_ERR_INVALID_ARG,
        TAG,
        "invalid led mode"
    );

    ESP_RETURN_ON_FALSE(
        color >= R503_LED_RED && color <= R503_LED_WHITE,
        ESP_ERR_INVALID_ARG,
        TAG,
        "invalid led color"
    );

    uint8_t params[4] = {
        (uint8_t)mode,
        speed,
        (uint8_t)color,
        times,
    };

    ESP_RETURN_ON_ERROR(
        r503_send_command(dev, R503_CMD_AURA_LED, params, sizeof(params)),
        TAG,
        "send aura led config failed"
    );

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read aura led ack failed");

    return r503_map_confirm_code(code);
}
