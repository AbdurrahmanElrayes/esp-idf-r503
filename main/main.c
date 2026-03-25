#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "r503.h"
#include "r503_defs.h"

static const char *TAG = "main";

void app_main(void)
{
    r503_t sensor = {0};

    r503_config_t cfg = {
        .uart_num = UART_NUM_1,
        .tx_pin = GPIO_NUM_17,
        .rx_pin = GPIO_NUM_18,
        .baud_rate = R503_DEFAULT_BAUDRATE,
        .address = R503_DEFAULT_ADDRESS,
        .password = R503_DEFAULT_PASSWORD,
        .rx_timeout_ms = 1000,
        .uart_buffer_size = 512,
    };

    ESP_ERROR_CHECK(r503_init(&sensor, &cfg));

    ESP_LOGI(TAG, "Waiting for R503 ready byte...");
    esp_err_t err = r503_wait_ready(&sensor, 1500);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Ready byte not received: %s", r503_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "R503 ready");
    }

    err = r503_handshake(&sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Handshake failed: %s", r503_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Handshake OK");

    err = r503_check_sensor(&sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Sensor check failed: %s", r503_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Sensor check OK");

    err = r503_verify_password(&sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Verify password failed: %s", r503_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Password verified");

    r503_sys_params_t params = {0};
    err = r503_read_sys_params(&sensor, &params);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read sys params failed: %s", r503_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Status reg      : 0x%04X", params.status_register);
    ESP_LOGI(TAG, "System ID       : 0x%04X", params.system_id);
    ESP_LOGI(TAG, "Capacity        : %u", params.capacity);
    ESP_LOGI(TAG, "Security level  : %u", params.security_level);
    ESP_LOGI(TAG, "Device address  : 0x%08" PRIX32, params.device_address);
    ESP_LOGI(TAG, "Packet size code: %u", params.packet_size_code);
    ESP_LOGI(TAG, "Baud multiplier : %u", params.baud_multiplier);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}