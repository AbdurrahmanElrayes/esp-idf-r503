#include <string.h>

#include "driver/uart.h"
#include "esp_check.h"
#include "esp_log.h"

#include "r503_defs.h"
#include "r503_errors.h"
#include "r503_types.h"

static const char *TAG = "r503_uart";

esp_err_t r503_uart_init(const r503_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev is NULL");

    uart_config_t uart_config = {
        .baud_rate = dev->cfg.baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(
        dev->cfg.uart_num,
        dev->cfg.uart_buffer_size,
        0,
        0,
        NULL,
        0
    );
    ESP_RETURN_ON_ERROR(err, TAG, "uart_driver_install failed");

    err = uart_param_config(dev->cfg.uart_num, &uart_config);
    if (err != ESP_OK) {
        uart_driver_delete(dev->cfg.uart_num);
        ESP_RETURN_ON_ERROR(err, TAG, "uart_param_config failed");
    }

    err = uart_set_pin(
        dev->cfg.uart_num,
        dev->cfg.tx_pin,
        dev->cfg.rx_pin,
        UART_PIN_NO_CHANGE,
        UART_PIN_NO_CHANGE
    );
    if (err != ESP_OK) {
        uart_driver_delete(dev->cfg.uart_num);
        ESP_RETURN_ON_ERROR(err, TAG, "uart_set_pin failed");
    }

    uart_flush_input(dev->cfg.uart_num);
    return ESP_OK;
}

esp_err_t r503_uart_deinit(const r503_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev is NULL");
    return uart_driver_delete(dev->cfg.uart_num);
}

esp_err_t r503_uart_write(const r503_t *dev, const uint8_t *data, size_t len)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev is NULL");
    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG, "data is NULL");

    int written = uart_write_bytes(dev->cfg.uart_num, data, len);
    ESP_RETURN_ON_FALSE(written >= 0, ESP_FAIL, TAG, "uart_write_bytes failed");
    ESP_RETURN_ON_FALSE((size_t)written == len, ESP_FAIL, TAG, "short write");

    return uart_wait_tx_done(dev->cfg.uart_num, pdMS_TO_TICKS(dev->cfg.rx_timeout_ms));
}

esp_err_t r503_uart_read_exact(const r503_t *dev, uint8_t *data, size_t len, uint32_t timeout_ms)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev is NULL");
    ESP_RETURN_ON_FALSE(data != NULL, ESP_ERR_INVALID_ARG, TAG, "data is NULL");

    size_t total = 0;
    const TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while (total < len) {
        int ret = uart_read_bytes(dev->cfg.uart_num, data + total, len - total, timeout_ticks);
        if (ret < 0) {
            return ESP_FAIL;
        }
        if (ret == 0) {
            return ESP_ERR_TIMEOUT;
        }
        total += (size_t)ret;
    }

    return ESP_OK;
}

esp_err_t r503_uart_flush_input(const r503_t *dev)
{
    ESP_RETURN_ON_FALSE(dev != NULL, ESP_ERR_INVALID_ARG, TAG, "dev is NULL");
    return uart_flush_input(dev->cfg.uart_num);
}