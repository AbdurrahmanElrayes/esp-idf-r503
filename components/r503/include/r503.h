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

const char *r503_err_to_name(esp_err_t err);

#ifdef __cplusplus
}
#endif