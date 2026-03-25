#pragma once

#include "esp_err.h"
#include "r503_types.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t r503_init(r503_t *dev, const r503_config_t *config);
esp_err_t r503_deinit(r503_t *dev);

esp_err_t r503_wait_ready(r503_t *dev, uint32_t timeout_ms);
esp_err_t r503_handshake(r503_t *dev);
esp_err_t r503_check_sensor(r503_t *dev);
esp_err_t r503_verify_password(r503_t *dev);
esp_err_t r503_read_sys_params(r503_t *dev, r503_sys_params_t *out);
esp_err_t r503_get_image(r503_t *dev);
esp_err_t r503_get_image_ex(r503_t *dev);
esp_err_t r503_gen_char(r503_t *dev, uint8_t buffer_id);
esp_err_t r503_template_count(r503_t *dev, uint16_t *count);
esp_err_t r503_reg_model(r503_t *dev);
esp_err_t r503_store(r503_t *dev, uint8_t buffer_id, uint16_t model_id);
esp_err_t r503_search(r503_t *dev,
                      uint8_t buffer_id,
                      uint16_t start_id,
                      uint16_t count,
                      r503_search_result_t *out);

const char *r503_err_to_name(esp_err_t err);

esp_err_t r503_enroll_manual(r503_t *dev, const r503_enroll_config_t *cfg);
esp_err_t r503_identify_once(r503_t *dev,
                             uint16_t start_id,
                             uint16_t count,
                             r503_search_result_t *out);
 
esp_err_t r503_delete(r503_t *dev, uint16_t start_id, uint16_t count);
esp_err_t r503_empty_library(r503_t *dev);
esp_err_t r503_read_index_table(r503_t *dev, uint8_t page_index, r503_index_table_t *out);
esp_err_t r503_find_next_free_id(r503_t *dev, uint16_t *out_id);

esp_err_t r503_enroll_manual_next_free(r503_t *dev,
                                       uint16_t *saved_id,
                                       uint32_t capture_timeout_ms,
                                       uint32_t poll_delay_ms);
                                       
#ifdef __cplusplus
}
#endif