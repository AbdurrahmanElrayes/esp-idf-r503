/*
 * Copyright 2026 Abdelrahman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "r503_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* lifecycle */
esp_err_t r503_init(r503_t *dev, const r503_config_t *config);
esp_err_t r503_deinit(r503_t *dev);

/* utility */
const char *r503_err_to_name(esp_err_t err);

/* basic communication */
esp_err_t r503_wait_ready(r503_t *dev, uint32_t timeout_ms);
esp_err_t r503_handshake(r503_t *dev);
esp_err_t r503_check_sensor(r503_t *dev);
esp_err_t r503_verify_password(r503_t *dev);

/* device/system info */
esp_err_t r503_read_sys_params(r503_t *dev, r503_sys_params_t *out);
esp_err_t r503_template_count(r503_t *dev, uint16_t *count);
esp_err_t r503_get_fw_version(r503_t *dev, r503_fw_version_t *out);
esp_err_t r503_get_alg_version(r503_t *dev, r503_alg_version_t *out);
esp_err_t r503_read_product_info(r503_t *dev, r503_product_info_t *out);

/* image / feature */
esp_err_t r503_get_image(r503_t *dev);
esp_err_t r503_get_image_ex(r503_t *dev);
esp_err_t r503_gen_char(r503_t *dev, uint8_t buffer_id);
esp_err_t r503_reg_model(r503_t *dev);

/* template management */
esp_err_t r503_store(r503_t *dev, uint8_t buffer_id, uint16_t model_id);
esp_err_t r503_search(r503_t *dev,
                      uint8_t buffer_id,
                      uint16_t start_id,
                      uint16_t count,
                      r503_search_result_t *out);
esp_err_t r503_delete(r503_t *dev, uint16_t start_id, uint16_t count);
esp_err_t r503_empty_library(r503_t *dev);

/* index / allocation helpers */
esp_err_t r503_read_index_table(r503_t *dev, uint8_t page_index, r503_index_table_t *out);
esp_err_t r503_find_next_free_id(r503_t *dev, uint16_t *out_id);

/* high-level helpers */
esp_err_t r503_enroll_manual(r503_t *dev, const r503_enroll_config_t *cfg);
esp_err_t r503_enroll_manual_next_free(r503_t *dev,
                                       uint16_t *saved_id,
                                       uint32_t capture_timeout_ms,
                                       uint32_t poll_delay_ms);
esp_err_t r503_identify_once(r503_t *dev,
                             uint16_t start_id,
                             uint16_t count,
                             r503_search_result_t *out);

/* aura led */
esp_err_t r503_aura_led_config(r503_t *dev,
                               r503_led_mode_t mode,
                               uint8_t speed,
                               r503_led_color_t color,
                               uint8_t times);

esp_err_t r503_auto_enroll(r503_t *dev,
                           uint16_t location_id,
                           bool allow_override,
                           bool allow_duplicate,
                           bool return_step_status,
                           bool require_finger_leave,
                           uint16_t *saved_id,
                           r503_auto_enroll_callback_t cb,
                           void *user_ctx);

esp_err_t r503_auto_identify(r503_t *dev,
                             uint8_t security_level,
                             uint16_t start_id,
                             uint16_t count,
                             bool return_step_status,
                             uint8_t search_error_times,
                             r503_search_result_t *out,
                             r503_auto_identify_callback_t cb,
                             void *user_ctx);

#ifdef __cplusplus
}
#endif