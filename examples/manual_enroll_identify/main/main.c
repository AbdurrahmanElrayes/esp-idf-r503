#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "r503.h"
#include "r503_defs.h"
#include "r503_errors.h"

static const char *TAG = "manual_example";

static esp_err_t wait_for_finger_and_gen_char(r503_t *sensor, uint8_t buffer_id)
{
    while (1) {
        esp_err_t err = r503_get_image_ex(sensor);

        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Image captured");
            err = r503_gen_char(sensor, buffer_id);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "GenChar(%u) failed: %s", buffer_id, r503_err_to_name(err));
                return err;
            }

            ESP_LOGI(TAG, "Feature stored in CharBuffer%u", buffer_id);
            return ESP_OK;
        }

        if (err != ESP_ERR_R503_NO_FINGER) {
            ESP_LOGE(TAG, "Get image failed: %s", r503_err_to_name(err));
            return err;
        }

        vTaskDelay(pdMS_TO_TICKS(150));
    }
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

    ESP_LOGI(TAG, "Initializing sensor...");
    ESP_ERROR_CHECK(r503_init(&sensor, &cfg));

    if (r503_wait_ready(&sensor, 1500) == ESP_OK) {
        ESP_LOGI(TAG, "Sensor ready");
    } else {
        ESP_LOGW(TAG, "Ready byte not received (normal on some modules)");
    }

    ESP_ERROR_CHECK(r503_handshake(&sensor));
    ESP_ERROR_CHECK(r503_check_sensor(&sensor));
    ESP_ERROR_CHECK(r503_verify_password(&sensor));

    ESP_LOGI(TAG, "Sensor initialized successfully");

    r503_sys_params_t params = {0};
    ESP_ERROR_CHECK(r503_read_sys_params(&sensor, &params));

    uint16_t templates = 0;
    ESP_ERROR_CHECK(r503_template_count(&sensor, &templates));
    ESP_LOGI(TAG, "Stored templates: %u", templates);

    uint16_t next_id = 0;
    ESP_ERROR_CHECK(r503_find_next_free_id(&sensor, &next_id));
    ESP_LOGI(TAG, "Next free ID: %u", next_id);

    /* ===== MANUAL ENROLL ===== */

    ESP_LOGI(TAG, "=== MANUAL ENROLL START ===");
    ESP_LOGI(TAG, "Place the SAME finger (first capture)...");
    ESP_ERROR_CHECK(wait_for_finger_and_gen_char(&sensor, 1));

    wait_finger_removed(&sensor);

    ESP_LOGI(TAG, "Place the SAME finger again (second capture)...");
    ESP_ERROR_CHECK(wait_for_finger_and_gen_char(&sensor, 2));

    ESP_ERROR_CHECK(r503_reg_model(&sensor));
    ESP_LOGI(TAG, "Template merged successfully");

    ESP_ERROR_CHECK(r503_store(&sensor, 1, next_id));
    ESP_LOGI(TAG, "Template stored at ID %u", next_id);

    wait_finger_removed(&sensor);

    /* ===== MANUAL IDENTIFY ===== */

    ESP_LOGI(TAG, "=== MANUAL IDENTIFY START ===");
    ESP_LOGI(TAG, "Place enrolled finger for search...");

    ESP_ERROR_CHECK(wait_for_finger_and_gen_char(&sensor, 1));

    r503_search_result_t result = {0};
    esp_err_t err = r503_search(&sensor, 1, 0, params.capacity, &result);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Search failed: %s", r503_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "MATCH -> ID=%u Score=%u", result.match_id, result.match_score);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}