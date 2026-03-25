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

static const char *TAG = "r503_info";

void r503_copy_ascii_field(char *dst, size_t dst_size, const uint8_t *src, size_t src_len)
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
