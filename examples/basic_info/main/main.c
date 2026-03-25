#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "r503.h"
#include "r503_defs.h"

static const char *TAG = "basic_info";

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

    esp_err_t err = r503_wait_ready(&sensor, 1500);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Sensor ready");
    } else {
        ESP_LOGW(TAG, "Ready byte not received: %s", r503_err_to_name(err));
    }

    ESP_ERROR_CHECK(r503_handshake(&sensor));
    ESP_LOGI(TAG, "Handshake OK");

    ESP_ERROR_CHECK(r503_check_sensor(&sensor));
    ESP_LOGI(TAG, "Sensor check OK");

    ESP_ERROR_CHECK(r503_verify_password(&sensor));
    ESP_LOGI(TAG, "Password verified");

    r503_sys_params_t params = {0};
    ESP_ERROR_CHECK(r503_read_sys_params(&sensor, &params));

    ESP_LOGI(TAG, "=== System Parameters ===");
    ESP_LOGI(TAG, "Status reg      : 0x%04X", params.status_register);
    ESP_LOGI(TAG, "System ID       : 0x%04X", params.system_id);
    ESP_LOGI(TAG, "Capacity        : %u", params.capacity);
    ESP_LOGI(TAG, "Security level  : %u", params.security_level);
    ESP_LOGI(TAG, "Device address  : 0x%08" PRIX32, params.device_address);
    ESP_LOGI(TAG, "Packet size code: %u", params.packet_size_code);
    ESP_LOGI(TAG, "Baud multiplier : %u", params.baud_multiplier);

    uint16_t templates = 0;
    err = r503_template_count(&sensor, &templates);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Stored templates: %u", templates);
    } else {
        ESP_LOGE(TAG, "Template count failed: %s", r503_err_to_name(err));
    }

    r503_fw_version_t fw = {0};
    err = r503_get_fw_version(&sensor, &fw);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "FW Version      : %s", fw.fw_version);
    } else {
        ESP_LOGE(TAG, "Get FW version failed: %s", r503_err_to_name(err));
    }

    r503_alg_version_t alg = {0};
    err = r503_get_alg_version(&sensor, &alg);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Alg Version     : %s", alg.alg_version);
    } else {
        ESP_LOGE(TAG, "Get Alg version failed: %s", r503_err_to_name(err));
    }

    r503_product_info_t prod = {0};
    err = r503_read_product_info(&sensor, &prod);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "=== Product Info ===");
        ESP_LOGI(TAG, "Module Type     : %s", prod.module_type);
        ESP_LOGI(TAG, "Batch Number    : %s", prod.batch_number);
        ESP_LOGI(TAG, "Serial Number   : %s", prod.serial_number);
        ESP_LOGI(TAG, "HW Version      : %u.%u", prod.hw_ver_major, prod.hw_ver_minor);
        ESP_LOGI(TAG, "Sensor Type     : %s", prod.sensor_type);
        ESP_LOGI(TAG, "Sensor Size     : %ux%u", prod.sensor_width, prod.sensor_height);
        ESP_LOGI(TAG, "Template Size   : %u", prod.template_size);
        ESP_LOGI(TAG, "Template Total  : %u", prod.template_total);
    } else {
        ESP_LOGE(TAG, "Read product info failed: %s", r503_err_to_name(err));
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}