#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

#include "r503.h"
#include "r503_defs.h"
#include "r503_errors.h"

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
    }

    ESP_ERROR_CHECK(r503_handshake(&sensor));
    ESP_ERROR_CHECK(r503_check_sensor(&sensor));
    ESP_ERROR_CHECK(r503_verify_password(&sensor));

    r503_sys_params_t params = {0};
    ESP_ERROR_CHECK(r503_read_sys_params(&sensor, &params));

    ESP_LOGI(TAG, "Capacity        : %u", params.capacity);
    ESP_LOGI(TAG, "Security level  : %u", params.security_level);
    ESP_LOGI(TAG, "Device address  : 0x%08" PRIX32, params.device_address);

    uint16_t templates = 0;
    ESP_ERROR_CHECK(r503_template_count(&sensor, &templates));
    ESP_LOGI(TAG, "Stored templates : %u", templates);

    if (templates >= params.capacity) {
        ESP_LOGE(TAG, "Fingerprint library is full");
        return;
    }
// temp
    r503_index_table_t table0 = {0};
    err = r503_read_index_table(&sensor, 0, &table0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Read index table failed: %s", r503_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Index table page 0:");
        for (int i = 0; i < 32; i++) {
            ESP_LOGI(TAG, "  byte[%02d] = 0x%02X", i, table0.bits[i]);
        }
    }
    
    uint16_t next_free = 0;
    err = r503_find_next_free_id(&sensor, &next_free);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Find next free ID failed: %s", r503_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Next free ID: %u", next_free);
    }
// end temp

    uint16_t saved_id = 0;

    ESP_LOGI(TAG, "Enroll same finger twice now...");
    err = r503_enroll_manual_next_free(&sensor, &saved_id, 10000, 150);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Enroll failed: %s", r503_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Enroll success at ID %u", saved_id);

    r503_search_result_t result = {0};
    ESP_LOGI(TAG, "Place enrolled finger for identify...");
    err = r503_identify_once(&sensor, 0, params.capacity, &result);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Identify failed: %s", r503_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "MATCH! ID=%u Score=%u", result.match_id, result.match_score);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}