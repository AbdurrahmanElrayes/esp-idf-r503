/*
 * Copyright 2026 Abdelrahman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"

#include "r503_defs.h"
#include "r503_errors.h"
#include "r503_priv.h"

static const char *TAG = "r503_highlevel";

esp_err_t r503_wait_for_finger_and_gen_char(r503_t *dev,
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

esp_err_t r503_wait_finger_removed_internal(r503_t *dev,
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

esp_err_t r503_auto_enroll(r503_t *dev,
                           uint16_t location_id,
                           bool allow_override,
                           bool allow_duplicate,
                           bool return_step_status,
                           bool require_finger_leave,
                           uint16_t *saved_id,
                           r503_auto_enroll_callback_t cb,
                           void *user_ctx)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(saved_id != NULL, ESP_ERR_INVALID_ARG, TAG, "saved_id is NULL");

    uint8_t params[5] = {
        (uint8_t)(location_id & 0xFF), /* manual says ID byte here for 0..0xC8 use-case */
        allow_override ? 1 : 0,
        allow_duplicate ? 1 : 0,
        return_step_status ? 1 : 0,
        require_finger_leave ? 1 : 0,
    };

    ESP_RETURN_ON_ERROR(
        r503_send_command(dev, R503_CMD_AUTO_ENROLL, params, sizeof(params)),
        TAG,
        "send auto enroll failed"
    );

    while (1) {
        r503_packet_t ack = {0};
        uint8_t code = 0;

        ESP_RETURN_ON_ERROR(
            r503_read_ack_with_timeout(dev, &ack, &code, 15000),
            TAG,
            "read auto enroll ack failed"
        );

        const uint16_t payload_len = r503_packet_payload_len(&ack);
        ESP_RETURN_ON_FALSE(payload_len == 3, ESP_ERR_R503_PROTOCOL, TAG,
                            "unexpected auto enroll payload len: %u", payload_len);

        r503_auto_enroll_step_t event = {
            .confirmation_code = ack.payload[0],
            .step = ack.payload[1],
            .finger_id = ack.payload[2],
        };

        if (cb != NULL) {
            cb(&event, user_ctx);
        }

        if (event.confirmation_code != R503_CONFIRM_OK) {
            return r503_map_confirm_code(event.confirmation_code);
        }

        /* Final step according to manual: 0x0F = storage template */
        if (event.step == 0x0F) {
            *saved_id = event.finger_id;
            return ESP_OK;
        }

        if (!return_step_status) {
            /*
             * If step status reporting is disabled, some firmwares may only return
             * the final packet. In that case, any successful packet is treated as final.
             */
            *saved_id = event.finger_id;
            return ESP_OK;
        }
    }
}

esp_err_t r503_auto_identify(r503_t *dev,
                             uint8_t security_level,
                             uint16_t start_id,
                             uint16_t count,
                             bool return_step_status,
                             uint8_t search_error_times,
                             r503_search_result_t *out,
                             r503_auto_identify_callback_t cb,
                             void *user_ctx)
{
    ESP_RETURN_ON_ERROR(r503_check_dev(dev), TAG, "invalid device");
    ESP_RETURN_ON_FALSE(out != NULL, ESP_ERR_INVALID_ARG, TAG, "out is NULL");
    ESP_RETURN_ON_FALSE(security_level >= 1 && security_level <= 5,
                        ESP_ERR_INVALID_ARG, TAG, "security_level must be 1..5");

    /* This firmware uses 1-byte start_id and 1-byte count for AutoIdentify */
    ESP_RETURN_ON_FALSE(start_id <= 255, ESP_ERR_INVALID_ARG, TAG, "start_id must be <= 255");
    ESP_RETURN_ON_FALSE(count <= 255, ESP_ERR_INVALID_ARG, TAG, "count must be <= 255");

    uint8_t params[5] = {
        security_level,
        (uint8_t)start_id,
        (uint8_t)count,
        return_step_status ? 1 : 0,
        search_error_times,
    };

    ESP_RETURN_ON_ERROR(
        r503_send_command(dev, R503_CMD_AUTO_IDENTIFY, params, sizeof(params)),
        TAG,
        "send auto identify failed"
    );

    while (1) {
        r503_packet_t ack = {0};
        uint8_t code = 0;

        ESP_RETURN_ON_ERROR(
            r503_read_ack_with_timeout(dev, &ack, &code, 15000),
            TAG,
            "read auto identify ack failed"
        );

        const uint16_t payload_len = r503_packet_payload_len(&ack);

        if (payload_len < 1) {
            return ESP_ERR_R503_PROTOCOL;
        }

        if (ack.payload[0] != R503_CONFIRM_OK) {
            return r503_map_confirm_code(ack.payload[0]);
        }

        /* Some firmware versions first return only a short success ACK */
        if (payload_len == 1) {
            continue;
        }

        /*
         * Expected step packet payload:
         * [confirm][step][match_id_hi][match_id_lo][score_hi][score_lo]
         * => 6 bytes total payload
         */
        if (payload_len == 6) {
            r503_auto_identify_step_t event = {
                .confirmation_code = ack.payload[0],
                .step = ack.payload[1],
                .match_id = be16_from_payload(&ack.payload[2]),
                .match_score = be16_from_payload(&ack.payload[4]),
            };

            if (cb != NULL) {
                cb(&event, user_ctx);
            }

            if (event.step == 0x03) {
                out->match_id = event.match_id;
                out->match_score = event.match_score;
                return ESP_OK;
            }

            continue;
        }

        if (!return_step_status && payload_len >= 6) {
            out->match_id = be16_from_payload(&ack.payload[2]);
            out->match_score = be16_from_payload(&ack.payload[4]);
            return ESP_OK;
        }

        ESP_LOGE(TAG, "unexpected auto identify payload len: %u", payload_len);
        return ESP_ERR_R503_PROTOCOL;
    }
}
