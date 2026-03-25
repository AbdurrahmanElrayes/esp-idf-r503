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

static esp_err_t wait_for_finger_and_capture(r503_t *sensor, uint8_t buffer_id)
{
    esp_err_t err;

    while (1) {
        err = r503_get_image_ex(sensor);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Image captured");
            break;
        }

        if (err != ESP_ERR_R503_NO_FINGER) {
            ESP_LOGE(TAG, "Get image failed: %s", r503_err_to_name(err));
            return err;
        }

        vTaskDelay(pdMS_TO_TICKS(150));
    }

    err = r503_gen_char(sensor, buffer_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GenChar(%u) failed: %s", buffer_id, r503_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Feature stored in CharBuffer%u", buffer_id);
    return ESP_OK;
}

static void wait_finger_removed(r503_t *sensor)
{
    ESP_LOGI(TAG, "Remove finger...");

    while (1) {
        esp_err_t err = r503_get_image_ex(sensor);

        if (err == ESP_ERR_R503_NO_FINGER) {
            ESP_LOGI(TAG, "Finger removed");
            vTaskDelay(pdMS_TO_TICKS(500));
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

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

    uint16_t templates = 0;
    err = r503_template_count(&sensor, &templates);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Template count failed: %s", r503_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Stored templates : %u", templates);
    }

    if (templates >= params.capacity) {
        ESP_LOGE(TAG, "Fingerprint library is full");
        return;
    }

    uint16_t enroll_id = templates;

    ESP_LOGI(TAG, "=== ENROLL TEST START ===");
    ESP_LOGI(TAG, "Place the SAME finger (first capture)...");
    err = wait_for_finger_and_capture(&sensor, 1);
    if (err != ESP_OK) {
        return;
    }

    wait_finger_removed(&sensor);

    ESP_LOGI(TAG, "Place the SAME finger again (second capture)...");
    err = wait_for_finger_and_capture(&sensor, 2);
    if (err != ESP_OK) {
        return;
    }

    err = r503_reg_model(&sensor);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "RegModel failed: %s", r503_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Template merged successfully");

    err = r503_store(&sensor, 1, enroll_id);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Store failed: %s", r503_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "Template stored at ID %u", enroll_id);

    wait_finger_removed(&sensor);

    ESP_LOGI(TAG, "=== SEARCH TEST START ===");
    ESP_LOGI(TAG, "Place enrolled finger for search...");
    err = wait_for_finger_and_capture(&sensor, 1);
    if (err != ESP_OK) {
        return;
    }

    r503_search_result_t result = {0};
    err = r503_search(&sensor, 1, 0, params.capacity, &result);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Search failed: %s", r503_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "MATCH! ID=%u Score=%u", result.match_id, result.match_score);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}