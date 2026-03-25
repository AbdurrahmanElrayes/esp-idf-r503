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
#include <stdio.h>

#include "esp_check.h"
#include "esp_log.h"

#include "r503.h"
#include "r503_defs.h"
#include "r503_errors.h"
#include "r503_types.h"

/* internal UART functions */
esp_err_t r503_uart_init(const r503_t *dev);
esp_err_t r503_uart_deinit(const r503_t *dev);
esp_err_t r503_uart_write(const r503_t *dev, const uint8_t *data, size_t len);
esp_err_t r503_uart_read_exact(const r503_t *dev, uint8_t *data, size_t len, uint32_t timeout_ms);
esp_err_t r503_uart_flush_input(const r503_t *dev);

/* internal packet functions */
uint16_t r503_packet_checksum(uint8_t pid, uint16_t length, const uint8_t *payload, size_t payload_len);
esp_err_t r503_packet_build(uint32_t address,
                            uint8_t pid,
                            const uint8_t *payload,
                            size_t payload_len,
                            uint8_t *out_buf,
                            size_t out_buf_size,
                            size_t *out_len);
esp_err_t r503_packet_parse(const uint8_t *raw, size_t raw_len, r503_packet_t *packet);
uint16_t r503_packet_payload_len(const r503_packet_t *packet);

static const char *TAG = "r503";

static esp_err_t r503_map_confirm_code(uint8_t code)
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

static esp_err_t r503_check_dev(const r503_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev is NULL");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "device not initialized");
    return ESP_OK;
}

static esp_err_t r503_send_command(const r503_t *dev,
                                   uint8_t command,
                                   const uint8_t *params,
                                   size_t params_len)
{
    uint8_t payload[257] = {0};
    uint8_t packet[300] = {0};
    size_t packet_len = 0;

    ESP_RETURN_ON_FALSE(params_len <= 256, ESP_ERR_INVALID_ARG, TAG, "params too large");

    payload[0] = command;
    if (params_len > 0 && params != NULL) {
        memcpy(&payload[1], params, params_len);
    }

    ESP_RETURN_ON_ERROR(
        r503_packet_build(dev->cfg.address,
                          R503_PID_COMMAND,
                          payload,
                          params_len + 1,
                          packet,
                          sizeof(packet),
                          &packet_len),
        TAG,
        "packet build failed"
    );

    return r503_uart_write(dev, packet, packet_len);
}

static esp_err_t r503_read_packet(const r503_t *dev, r503_packet_t *packet, uint32_t timeout_ms)
{
    uint8_t header_and_meta[9] = {0};
    uint8_t rest[258] = {0}; /* max length field is payload(256)+checksum(2) */
    uint8_t full[267] = {0};

    ESP_RETURN_ON_ERROR(r503_uart_read_exact(dev, header_and_meta, sizeof(header_and_meta), timeout_ms),
                        TAG, "failed reading packet header");

    const uint16_t length = ((uint16_t)header_and_meta[7] << 8) | header_and_meta[8];
    ESP_RETURN_ON_FALSE(length >= 2, ESP_ERR_R503_PACKET, TAG, "invalid packet length");
    ESP_RETURN_ON_FALSE(length <= sizeof(rest), ESP_ERR_R503_PACKET, TAG, "packet length too large");

    ESP_RETURN_ON_ERROR(r503_uart_read_exact(dev, rest, length, timeout_ms),
                        TAG, "failed reading packet body");

    memcpy(full, header_and_meta, sizeof(header_and_meta));
    memcpy(full + sizeof(header_and_meta), rest, length);

    return r503_packet_parse(full, sizeof(header_and_meta) + length, packet);
}

static esp_err_t r503_read_ack(const r503_t *dev, r503_packet_t *packet, uint8_t *confirm_code)
{
    ESP_RETURN_ON_ERROR(r503_read_packet(dev, packet, dev->cfg.rx_timeout_ms), TAG, "read packet failed");
    ESP_RETURN_ON_FALSE(packet->pid == R503_PID_ACK, ESP_ERR_R503_PROTOCOL, TAG, "expected ACK packet");

    const uint16_t payload_len = r503_packet_payload_len(packet);
    ESP_RETURN_ON_FALSE(payload_len >= 1, ESP_ERR_R503_PROTOCOL, TAG, "ACK payload too short");

    if (confirm_code != NULL) {
        *confirm_code = packet->payload[0];
    }

    return ESP_OK;
}

static uint16_t be16_from_payload(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static uint32_t be32_from_payload(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           ((uint32_t)p[3]);
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

esp_err_t r503_wait_ready(r503_t *dev, uint32_t timeout_ms)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");

    uint8_t byte = 0;
    esp_err_t err = r503_uart_read_exact(dev, &byte, 1, timeout_ms);
    if (err == ESP_ERR_TIMEOUT) {
        return ESP_ERR_R503_NOT_READY;
    }
    ESP_RETURN_ON_ERROR(err, TAG, "failed reading ready byte");

    ESP_RETURN_ON_FALSE(byte == R503_READY_BYTE, ESP_ERR_R503_NOT_READY, TAG, "unexpected ready byte 0x%02X", byte);
    return ESP_OK;
}

esp_err_t r503_handshake(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_HANDSHAKE, NULL, 0), TAG, "send handshake failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read handshake ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_check_sensor(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_CHECK_SENSOR, NULL, 0), TAG, "send check sensor failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read check sensor ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_verify_password(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");

    uint8_t params[4] = {
        (uint8_t)((dev->cfg.password >> 24) & 0xFF),
        (uint8_t)((dev->cfg.password >> 16) & 0xFF),
        (uint8_t)((dev->cfg.password >> 8) & 0xFF),
        (uint8_t)(dev->cfg.password & 0xFF),
    };

    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_VFY_PWD, params, sizeof(params)),
                        TAG, "send verify password failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read verify password ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_read_sys_params(r503_t *dev, r503_sys_params_t *out)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "out is NULL");

    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_READ_SYS_PARA, NULL, 0),
                        TAG, "send read sys params failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read sys params ack failed");

    esp_err_t mapped = r503_map_confirm_code(code);
    ESP_RETURN_ON_ERROR(mapped, TAG, "module returned error 0x%02X", code);

    const uint16_t payload_len = r503_packet_payload_len(&ack);
    ESP_RETURN_ON_FALSE(payload_len == 17, ESP_ERR_R503_PROTOCOL, TAG, "unexpected sys params payload len: %u", payload_len);

    const uint8_t *p = &ack.payload[1]; /* skip confirmation code */

    out->status_register  = be16_from_payload(&p[0]);
    out->system_id        = be16_from_payload(&p[2]);
    out->capacity         = be16_from_payload(&p[4]);
    out->security_level   = be16_from_payload(&p[6]);
    out->device_address   = be32_from_payload(&p[8]);
    out->packet_size_code = be16_from_payload(&p[12]);
    out->baud_multiplier  = be16_from_payload(&p[14]);

    return ESP_OK;
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

esp_err_t r503_get_image(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_GET_IMAGE, NULL, 0), TAG, "send get image failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read get image ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_get_image_ex(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_GET_IMAGE_EX, NULL, 0), TAG, "send get image ex failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read get image ex ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_gen_char(r503_t *dev, uint8_t buffer_id)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(buffer_id >= 1 && buffer_id <= 6, ESP_ERR_INVALID_ARG, TAG, "buffer_id must be 1..6");

    uint8_t params[1] = { buffer_id };

    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_GEN_CHAR, params, sizeof(params)),
                        TAG, "send gen char failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read gen char ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_template_count(r503_t *dev, uint16_t *count)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(count != NULL, ESP_ERR_INVALID_ARG, TAG, "count is NULL");

    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_TEMPLATE_NUM, NULL, 0),
                        TAG, "send template count failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read template count ack failed");

    esp_err_t mapped = r503_map_confirm_code(code);
    ESP_RETURN_ON_ERROR(mapped, TAG, "module returned error 0x%02X", code);

    const uint16_t payload_len = r503_packet_payload_len(&ack);
    ESP_RETURN_ON_FALSE(payload_len == 3, ESP_ERR_R503_PROTOCOL, TAG, "unexpected template count payload len: %u", payload_len);

    *count = be16_from_payload(&ack.payload[1]);
    return ESP_OK;
}

esp_err_t r503_reg_model(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_REG_MODEL, NULL, 0), TAG, "send reg model failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read reg model ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_store(r503_t *dev, uint8_t buffer_id, uint16_t model_id)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(buffer_id >= 1 && buffer_id <= 6, ESP_ERR_INVALID_ARG, TAG, "buffer_id must be 1..6");

    uint8_t params[3] = {
        buffer_id,
        (uint8_t)((model_id >> 8) & 0xFF),
        (uint8_t)(model_id & 0xFF),
    };

    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_STORE, params, sizeof(params)),
                        TAG, "send store failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read store ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_search(r503_t *dev,
                      uint8_t buffer_id,
                      uint16_t start_id,
                      uint16_t count,
                      r503_search_result_t *out)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "out is NULL");
    ESP_RETURN_ON_FALSE(buffer_id >= 1 && buffer_id <= 6, ESP_ERR_INVALID_ARG, TAG, "buffer_id must be 1..6");

    uint8_t params[5] = {
        buffer_id,
        (uint8_t)((start_id >> 8) & 0xFF),
        (uint8_t)(start_id & 0xFF),
        (uint8_t)((count >> 8) & 0xFF),
        (uint8_t)(count & 0xFF),
    };

    ESP_RETURN_ON_ERROR(r503_send_command(dev, R503_CMD_SEARCH, params, sizeof(params)),
                        TAG, "send search failed");

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read search ack failed");

    esp_err_t mapped = r503_map_confirm_code(code);
    ESP_RETURN_ON_ERROR(mapped, TAG, "module returned error 0x%02X", code);

    const uint16_t payload_len = r503_packet_payload_len(&ack);
    ESP_RETURN_ON_FALSE(payload_len == 5, ESP_ERR_R503_PROTOCOL, TAG, "unexpected search payload len: %u", payload_len);

    out->match_id = be16_from_payload(&ack.payload[1]);
    out->match_score = be16_from_payload(&ack.payload[3]);

    return ESP_OK;
}

static esp_err_t r503_wait_for_finger_and_gen_char(r503_t *dev,
                                                   uint8_t buffer_id,
                                                   uint32_t capture_timeout_ms,
                                                   uint32_t poll_delay_ms)
{
    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout_ticks = pdMS_TO_TICKS(capture_timeout_ms);

    while (1) {
        esp_err_t err = r503_get_image_ex(dev);
        if (err == ESP_OK) {
            return r503_gen_char(dev, buffer_id);
        }

        if (err != ESP_ERR_R503_NO_FINGER) {
            return err;
        }

        if ((xTaskGetTickCount() - start) >= timeout_ticks) {
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(poll_delay_ms));
    }
}

static esp_err_t r503_wait_finger_removed_internal(r503_t *dev,
                                                   uint32_t timeout_ms,
                                                   uint32_t poll_delay_ms)
{
    const TickType_t start = xTaskGetTickCount();
    const TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while (1) {
        esp_err_t err = r503_get_image_ex(dev);
        if (err == ESP_ERR_R503_NO_FINGER) {
            vTaskDelay(pdMS_TO_TICKS(300));
            return ESP_OK;
        }

        if (err != ESP_OK) {
            return err;
        }

        if ((xTaskGetTickCount() - start) >= timeout_ticks) {
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(poll_delay_ms));
    }
}

esp_err_t r503_enroll_manual(r503_t *dev, const r503_enroll_config_t *cfg)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(cfg != NULL, ESP_ERR_INVALID_ARG, TAG, "cfg is NULL");

    const uint32_t capture_timeout_ms = cfg->capture_timeout_ms ? cfg->capture_timeout_ms : 10000;
    const uint32_t poll_delay_ms = cfg->poll_delay_ms ? cfg->poll_delay_ms : 150;

    ESP_RETURN_ON_ERROR(
        r503_wait_for_finger_and_gen_char(dev, 1, capture_timeout_ms, poll_delay_ms),
        TAG, "first capture failed"
    );

    ESP_RETURN_ON_ERROR(
        r503_wait_finger_removed_internal(dev, capture_timeout_ms, poll_delay_ms),
        TAG, "waiting finger removal failed"
    );

    ESP_RETURN_ON_ERROR(
        r503_wait_for_finger_and_gen_char(dev, 2, capture_timeout_ms, poll_delay_ms),
        TAG, "second capture failed"
    );

    ESP_RETURN_ON_ERROR(r503_reg_model(dev), TAG, "reg model failed");
    ESP_RETURN_ON_ERROR(r503_store(dev, 1, cfg->model_id), TAG, "store failed");

    return ESP_OK;
}

esp_err_t r503_identify_once(r503_t *dev,
                             uint16_t start_id,
                             uint16_t count,
                             r503_search_result_t *out)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "out is NULL");

    ESP_RETURN_ON_ERROR(
        r503_wait_for_finger_and_gen_char(dev, 1, 10000, 150),
        TAG, "capture for identify failed"
    );

    return r503_search(dev, 1, start_id, count, out);
}

esp_err_t r503_delete(r503_t *dev, uint16_t start_id, uint16_t count)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(count > 0, ESP_ERR_INVALID_ARG, TAG, "count must be > 0");

    uint8_t params[4] = {
        (uint8_t)((start_id >> 8) & 0xFF),
        (uint8_t)(start_id & 0xFF),
        (uint8_t)((count >> 8) & 0xFF),
        (uint8_t)(count & 0xFF),
    };

    ESP_RETURN_ON_ERROR(
        r503_send_command(dev, R503_CMD_DELETE_CHAR, params, sizeof(params)),
        TAG,
        "send delete failed"
    );

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read delete ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_empty_library(r503_t *dev)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");

    ESP_RETURN_ON_ERROR(
        r503_send_command(dev, R503_CMD_EMPTY, NULL, 0),
        TAG,
        "send empty library failed"
    );

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read empty library ack failed");

    return r503_map_confirm_code(code);
}

esp_err_t r503_read_index_table(r503_t *dev, uint8_t page_index, r503_index_table_t *out)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "out is NULL");
    ESP_RETURN_ON_FALSE(page_index <= 3, ESP_ERR_INVALID_ARG, TAG, "page_index must be 0..3");

    uint8_t params[1] = { page_index };

    ESP_RETURN_ON_ERROR(
        r503_send_command(dev, R503_CMD_READ_INDEX_TABLE, params, sizeof(params)),
        TAG,
        "send read index table failed"
    );

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read index table ack failed");

    esp_err_t mapped = r503_map_confirm_code(code);
    ESP_RETURN_ON_ERROR(mapped, TAG, "module returned error 0x%02X", code);

    const uint16_t payload_len = r503_packet_payload_len(&ack);
    ESP_RETURN_ON_FALSE(payload_len == 33, ESP_ERR_R503_PROTOCOL, TAG,
                        "unexpected index table payload len: %u", payload_len);

    out->page_index = page_index;
    memcpy(out->bits, &ack.payload[1], 32);

    return ESP_OK;
}

static bool r503_index_bit_is_set(const uint8_t *bits, uint16_t local_id)
{
    uint16_t byte_index = local_id / 8;
    uint8_t bit_index = local_id % 8;

    return ((bits[byte_index] >> bit_index) & 0x01) != 0;
}

esp_err_t r503_find_next_free_id(r503_t *dev, uint16_t *out_id)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(out_id != NULL, ESP_ERR_INVALID_ARG, TAG, "out_id is NULL");

    r503_sys_params_t params = {0};
    ESP_RETURN_ON_ERROR(r503_read_sys_params(dev, &params), TAG, "read sys params failed");

    uint16_t capacity = params.capacity;
    if (capacity == 0) {
        return ESP_ERR_R503_LIBRARY_EMPTY;
    }

    uint16_t max_pages = (capacity + 255) / 256;
    if (max_pages > 4) {
        max_pages = 4;
    }

    for (uint16_t page = 0; page < max_pages; page++) {
        r503_index_table_t table = {0};
        ESP_RETURN_ON_ERROR(r503_read_index_table(dev, (uint8_t)page, &table),
                            TAG, "read index table page %u failed", page);

        uint16_t page_base = page * 256;
        uint16_t page_limit = capacity - page_base;
        if (page_limit > 256) {
            page_limit = 256;
        }

        for (uint16_t local_id = 0; local_id < page_limit; local_id++) {
            if (!r503_index_bit_is_set(table.bits, local_id)) {
                *out_id = page_base + local_id;
                return ESP_OK;
            }
        }
    }

    return ESP_ERR_R503_LIBRARY_FULL;
}

esp_err_t r503_enroll_manual_next_free(r503_t *dev,
                                       uint16_t *saved_id,
                                       uint32_t capture_timeout_ms,
                                       uint32_t poll_delay_ms)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(saved_id != NULL, ESP_ERR_INVALID_ARG, TAG, "saved_id is NULL");

    uint16_t next_id = 0;
    ESP_RETURN_ON_ERROR(r503_find_next_free_id(dev, &next_id), TAG, "find next free id failed");

    r503_enroll_config_t cfg = {
        .model_id = next_id,
        .capture_timeout_ms = (uint16_t)capture_timeout_ms,
        .poll_delay_ms = (uint16_t)poll_delay_ms,
    };

    ESP_RETURN_ON_ERROR(r503_enroll_manual(dev, &cfg), TAG, "manual enroll failed");

    *saved_id = next_id;
    return ESP_OK;
}

static void r503_copy_ascii_field(char *dst, size_t dst_size, const uint8_t *src, size_t src_len)
{
    size_t n = (src_len < (dst_size - 1)) ? src_len : (dst_size - 1);
    memcpy(dst, src, n);
    dst[n] = '\0';

    for (size_t i = 0; i < n; i++) {
        if (dst[i] == '\0') {
            break;
        }
    }
}

esp_err_t r503_get_fw_version(r503_t *dev, r503_fw_version_t *out)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "out is NULL");

    ESP_RETURN_ON_ERROR(
        r503_send_command(dev, R503_CMD_GET_FW_VER, NULL, 0),
        TAG,
        "send get fw version failed"
    );

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read fw version ack failed");

    esp_err_t mapped = r503_map_confirm_code(code);
    ESP_RETURN_ON_ERROR(mapped, TAG, "module returned error 0x%02X", code);

    const uint16_t payload_len = r503_packet_payload_len(&ack);
    ESP_RETURN_ON_FALSE(payload_len == 33, ESP_ERR_R503_PROTOCOL, TAG,
                        "unexpected fw version payload len: %u", payload_len);

    memset(out, 0, sizeof(*out));
    r503_copy_ascii_field(out->fw_version, sizeof(out->fw_version), &ack.payload[1], 32);

    return ESP_OK;
}

esp_err_t r503_get_alg_version(r503_t *dev, r503_alg_version_t *out)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "out is NULL");

    ESP_RETURN_ON_ERROR(
        r503_send_command(dev, R503_CMD_GET_ALG_VER, NULL, 0),
        TAG,
        "send get alg version failed"
    );

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read alg version ack failed");

    esp_err_t mapped = r503_map_confirm_code(code);
    ESP_RETURN_ON_ERROR(mapped, TAG, "module returned error 0x%02X", code);

    const uint16_t payload_len = r503_packet_payload_len(&ack);
    ESP_RETURN_ON_FALSE(payload_len == 33, ESP_ERR_R503_PROTOCOL, TAG,
                        "unexpected alg version payload len: %u", payload_len);

    memset(out, 0, sizeof(*out));
    r503_copy_ascii_field(out->alg_version, sizeof(out->alg_version), &ack.payload[1], 32);

    return ESP_OK;
}

esp_err_t r503_read_product_info(r503_t *dev, r503_product_info_t *out)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "out is NULL");

    ESP_RETURN_ON_ERROR(
        r503_send_command(dev, R503_CMD_READ_PROD_INFO, NULL, 0),
        TAG,
        "send read product info failed"
    );

    r503_packet_t ack = {0};
    uint8_t code = 0;
    ESP_RETURN_ON_ERROR(r503_read_ack(dev, &ack, &code), TAG, "read product info ack failed");

    esp_err_t mapped = r503_map_confirm_code(code);
    ESP_RETURN_ON_ERROR(mapped, TAG, "module returned error 0x%02X", code);

    const uint16_t payload_len = r503_packet_payload_len(&ack);

    /* Accept both:
     * 47 = confirmation(1) + 46 bytes actual fields
     * 51 = confirmation(1) + 46 bytes actual fields + 4 reserved bytes
     */
    ESP_RETURN_ON_FALSE(payload_len == 47 || payload_len == 51,
                        ESP_ERR_R503_PROTOCOL,
                        TAG,
                        "unexpected product info payload len: %u",
                        payload_len);

    const uint8_t *p = &ack.payload[1]; /* skip confirmation code */

    memset(out, 0, sizeof(*out));

    r503_copy_ascii_field(out->module_type, sizeof(out->module_type), &p[0], 16);
    r503_copy_ascii_field(out->batch_number, sizeof(out->batch_number), &p[16], 4);
    r503_copy_ascii_field(out->serial_number, sizeof(out->serial_number), &p[20], 8);

    out->hw_ver_major = p[28];
    out->hw_ver_minor = p[29];

    r503_copy_ascii_field(out->sensor_type, sizeof(out->sensor_type), &p[30], 8);

    out->sensor_width   = be16_from_payload(&p[38]);
    out->sensor_height  = be16_from_payload(&p[40]);
    out->template_size  = be16_from_payload(&p[42]);
    out->template_total = be16_from_payload(&p[44]);

    return ESP_OK;
}

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