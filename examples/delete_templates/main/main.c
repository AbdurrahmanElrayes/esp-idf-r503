#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "r503.h"
#include "r503_defs.h"

static const char *TAG = "delete_demo";

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

    ESP_LOGI(TAG, "Initializing sensor...");
    ESP_ERROR_CHECK(r503_init(&sensor, &cfg));

    if (r503_wait_ready(&sensor, 1500) == ESP_OK) {
        ESP_LOGI(TAG, "Sensor ready");
    }

    ESP_ERROR_CHECK(r503_handshake(&sensor));
    ESP_ERROR_CHECK(r503_check_sensor(&sensor));
    ESP_ERROR_CHECK(r503_verify_password(&sensor));

    ESP_LOGI(TAG, "Reading template count...");

    uint16_t count = 0;
    ESP_ERROR_CHECK(r503_template_count(&sensor, &count));

    ESP_LOGI(TAG, "Templates before delete: %u", count);

    if (count == 0) {
        ESP_LOGW(TAG, "Library already empty");
    } else {
        ESP_LOGW(TAG, "Deleting ALL templates...");

        esp_err_t err = r503_empty_library(&sensor);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Empty library failed: %s", r503_err_to_name(err));
            return;
        }

        ESP_LOGI(TAG, "Library cleared successfully");
    }

    ESP_ERROR_CHECK(r503_template_count(&sensor, &count));
    ESP_LOGI(TAG, "Templates after delete: %u", count);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}