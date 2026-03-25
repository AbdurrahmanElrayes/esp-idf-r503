/*
 * Copyright 2026 Abdelrahman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include <string.h>

#include "esp_check.h"
#include "esp_log.h"

#include "r503_defs.h"
#include "r503_errors.h"
#include "r503_priv.h"

static const char *TAG = "r503";

esp_err_t r503_map_confirm_code(uint8_t code)
{
    switch (code) {
        case R503_CONFIRM_OK:
            return ESP_OK;

        case R503_CONFIRM_PACKET_ERROR:
            return ESP_ERR_R503_PACKET;

        case R503_CONFIRM_NO_FINGER:
            return ESP_ERR_R503_NO_FINGER;

        case R503_CONFIRM_ENROLL_FAIL:
            return ESP_ERR_R503_ENROLL_FAIL;

        case R503_CONFIRM_IMAGE_MESSY:
        case R503_CONFIRM_FEATURE_FAIL:
        case R503_CONFIRM_INVALID_IMAGE:
            return ESP_ERR_R503_BAD_IMAGE;

        case R503_CONFIRM_NO_MATCH:
            return ESP_ERR_R503_NO_MATCH;

        case R503_CONFIRM_NOT_FOUND:
            return ESP_ERR_R503_NOT_FOUND;

        case R503_CONFIRM_MERGE_FAIL:
            return ESP_ERR_R503_MERGE_FAIL;

        case R503_CONFIRM_BAD_LOCATION:
            return ESP_ERR_R503_BAD_LOCATION;

        case R503_CONFIRM_LOAD_FAIL:
            return ESP_ERR_R503_LOAD_FAIL;

        case R503_CONFIRM_UPLOAD_FAIL:
        case R503_CONFIRM_UPLOAD_IMAGE_FAIL:
            return ESP_ERR_R503_UPLOAD_FAIL;

        case R503_CONFIRM_RECEIVE_FAIL:
            return ESP_ERR_R503_RECEIVE_FAIL;

        case R503_CONFIRM_DELETE_FAIL:
            return ESP_ERR_R503_DELETE_FAIL;

        case R503_CONFIRM_EMPTY_FAIL:
            return ESP_ERR_R503_EMPTY_FAIL;

        case R503_CONFIRM_WRONG_PASSWORD:
            return ESP_ERR_R503_WRONG_PASSWORD;

        case R503_CONFIRM_FLASH_ERROR:
            return ESP_ERR_R503_FLASH;

        case R503_CONFIRM_INVALID_REGISTER:
            return ESP_ERR_R503_INVALID_REGISTER;

        case R503_CONFIRM_BAD_REGISTER_CONF:
            return ESP_ERR_R503_BAD_REGISTER_CONF;

        case R503_CONFIRM_BAD_NOTEPAD_PAGE:
            return ESP_ERR_R503_BAD_NOTEPAD_PAGE;

        case R503_CONFIRM_COMM_ERROR:
            return ESP_ERR_R503_COMM_ERROR;

        case R503_CONFIRM_ADDRESS_ERROR:
            return ESP_ERR_R503_BAD_LOCATION;

        case R503_CONFIRM_PASSWORD_REQUIRED:
            return ESP_ERR_R503_PASSWORD_REQUIRED;

        case R503_CONFIRM_TEMPLATE_EMPTY:
            return ESP_ERR_R503_TEMPLATE_EMPTY;

        case R503_CONFIRM_LIBRARY_EMPTY:
            return ESP_ERR_R503_LIBRARY_EMPTY;

        case R503_CONFIRM_TIMEOUT:
            return ESP_ERR_R503_TIMEOUT;

        case R503_CONFIRM_ALREADY_EXISTS:
            return ESP_ERR_R503_ALREADY_EXISTS;

        case R503_CONFIRM_SENSOR_ERROR:
            return ESP_ERR_R503_SENSOR_ERROR;

        case R503_CONFIRM_LIBRARY_FULL:
            return ESP_ERR_R503_LIBRARY_FULL;

        case R503_CONFIRM_UNSUPPORTED:
            return ESP_ERR_R503_UNSUPPORTED;

        case R503_CONFIRM_HW_ERROR:
            return ESP_ERR_R503_HW_ERROR;

        case R503_CONFIRM_EXEC_FAIL:
            return ESP_ERR_R503_EXEC_FAIL;

        default:
            return ESP_ERR_R503_ACK;
    }
}

esp_err_t r503_init(r503_t *dev, const r503_config_t *config)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev is NULL");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");

    memset(dev, 0, sizeof(*dev));
    dev->cfg = *config;

    if (dev->cfg.baud_rate <= 0) {
        dev->cfg.baud_rate = R503_DEFAULT_BAUDRATE;
    }

    if (dev->cfg.address == 0) {
        dev->cfg.address = R503_DEFAULT_ADDRESS;
    }

    if (dev->cfg.rx_timeout_ms == 0) {
        dev->cfg.rx_timeout_ms = R503_DEFAULT_TIMEOUT_MS;
    }

    if (dev->cfg.uart_buffer_size <= 0) {
        dev->cfg.uart_buffer_size = R503_DEFAULT_UART_BUF_SIZE;
    }

    ESP_RETURN_ON_ERROR(r503_uart_init(dev), TAG, "uart init failed");

    dev->initialized = true;
    return ESP_OK;
}

esp_err_t r503_deinit(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");

    esp_err_t err = r503_uart_deinit(dev);
    dev->initialized = false;
    return err;
}

const char *r503_err_to_name(esp_err_t err)
{
    switch (err) {
        case ESP_OK:                          return "ESP_OK";
        case ESP_ERR_R503_PACKET:            return "ESP_ERR_R503_PACKET";
        case ESP_ERR_R503_ACK:               return "ESP_ERR_R503_ACK";
        case ESP_ERR_R503_NO_FINGER:         return "ESP_ERR_R503_NO_FINGER";
        case ESP_ERR_R503_BAD_IMAGE:         return "ESP_ERR_R503_BAD_IMAGE";
        case ESP_ERR_R503_NO_MATCH:          return "ESP_ERR_R503_NO_MATCH";
        case ESP_ERR_R503_NOT_FOUND:         return "ESP_ERR_R503_NOT_FOUND";
        case ESP_ERR_R503_WRONG_PASSWORD:    return "ESP_ERR_R503_WRONG_PASSWORD";
        case ESP_ERR_R503_FLASH:             return "ESP_ERR_R503_FLASH";
        case ESP_ERR_R503_TIMEOUT:           return "ESP_ERR_R503_TIMEOUT";
        case ESP_ERR_R503_ALREADY_EXISTS:    return "ESP_ERR_R503_ALREADY_EXISTS";
        case ESP_ERR_R503_SENSOR_ERROR:      return "ESP_ERR_R503_SENSOR_ERROR";
        case ESP_ERR_R503_LIBRARY_FULL:      return "ESP_ERR_R503_LIBRARY_FULL";
        case ESP_ERR_R503_LIBRARY_EMPTY:     return "ESP_ERR_R503_LIBRARY_EMPTY";
        case ESP_ERR_R503_BAD_LOCATION:      return "ESP_ERR_R503_BAD_LOCATION";
        case ESP_ERR_R503_NOT_READY:         return "ESP_ERR_R503_NOT_READY";
        case ESP_ERR_R503_PROTOCOL:          return "ESP_ERR_R503_PROTOCOL";

        case ESP_ERR_R503_ENROLL_FAIL:       return "ESP_ERR_R503_ENROLL_FAIL";
        case ESP_ERR_R503_MERGE_FAIL:        return "ESP_ERR_R503_MERGE_FAIL";
        case ESP_ERR_R503_LOAD_FAIL:         return "ESP_ERR_R503_LOAD_FAIL";
        case ESP_ERR_R503_UPLOAD_FAIL:       return "ESP_ERR_R503_UPLOAD_FAIL";
        case ESP_ERR_R503_RECEIVE_FAIL:      return "ESP_ERR_R503_RECEIVE_FAIL";
        case ESP_ERR_R503_DELETE_FAIL:       return "ESP_ERR_R503_DELETE_FAIL";
        case ESP_ERR_R503_EMPTY_FAIL:        return "ESP_ERR_R503_EMPTY_FAIL";
        case ESP_ERR_R503_PASSWORD_REQUIRED: return "ESP_ERR_R503_PASSWORD_REQUIRED";
        case ESP_ERR_R503_TEMPLATE_EMPTY:    return "ESP_ERR_R503_TEMPLATE_EMPTY";
        case ESP_ERR_R503_INVALID_REGISTER:  return "ESP_ERR_R503_INVALID_REGISTER";
        case ESP_ERR_R503_BAD_REGISTER_CONF: return "ESP_ERR_R503_BAD_REGISTER_CONF";
        case ESP_ERR_R503_BAD_NOTEPAD_PAGE:  return "ESP_ERR_R503_BAD_NOTEPAD_PAGE";
        case ESP_ERR_R503_COMM_ERROR:        return "ESP_ERR_R503_COMM_ERROR";
        case ESP_ERR_R503_UNSUPPORTED:       return "ESP_ERR_R503_UNSUPPORTED";
        case ESP_ERR_R503_HW_ERROR:          return "ESP_ERR_R503_HW_ERROR";
        case ESP_ERR_R503_EXEC_FAIL:         return "ESP_ERR_R503_EXEC_FAIL";

        default:
            return esp_err_to_name(err);
    }
}
