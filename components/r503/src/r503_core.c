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

static const char *TAG = "r503_core";

esp_err_t r503_check_dev(const r503_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev is NULL");
    ESP_RETURN_ON_FALSE(dev->initialized, ESP_ERR_INVALID_STATE, TAG, "device not initialized");
    return ESP_OK;
}

esp_err_t r503_send_command(const r503_t *dev,
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

esp_err_t r503_read_packet(const r503_t *dev, r503_packet_t *packet, uint32_t timeout_ms)
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

esp_err_t r503_read_ack(const r503_t *dev, r503_packet_t *packet, uint8_t *confirm_code)
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

esp_err_t r503_read_ack_with_timeout(const r503_t *dev,
                                    r503_packet_t *packet,
                                    uint8_t *confirm_code,
                                    uint32_t timeout_ms)
{
    ESP_RETURN_ON_ERROR(r503_read_packet(dev, packet, timeout_ms), TAG, "read packet failed");
    ESP_RETURN_ON_FALSE(packet->pid == R503_PID_ACK, ESP_ERR_R503_PROTOCOL, TAG, "expected ACK packet");

    const uint16_t payload_len = r503_packet_payload_len(packet);
    ESP_RETURN_ON_FALSE(payload_len >= 1, ESP_ERR_R503_PROTOCOL, TAG, "ACK payload too short");

    if (confirm_code != NULL) {
        *confirm_code = packet->payload[0];
    }

    return ESP_OK;
}

uint16_t be16_from_payload(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

uint32_t be32_from_payload(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           ((uint32_t)p[3]);
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
