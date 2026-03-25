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

static const char *TAG = "r503_template";

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

bool r503_index_bit_is_set(const uint8_t *bits, uint16_t local_id)
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
