#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "r503.h"
#include "r503_defs.h"

static const char *TAG = "r503_example";

static void auto_enroll_cb(const r503_auto_enroll_step_t *event, void *user_ctx)
{
    (void)user_ctx;
    ESP_LOGI("auto_enroll", "code=0x%02X step=0x%02X id=%u",
             event->confirmation_code, event->step, event->finger_id);
}

static void auto_identify_cb(const r503_auto_identify_step_t *event, void *user_ctx)
{
    (void)user_ctx;
    ESP_LOGI("auto_identify", "code=0x%02X step=0x%02X id=%u score=%u",
             event->confirmation_code, event->step, event->match_id, event->match_score);
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

    esp_err_t err = r503_wait_ready(&sensor, 1500);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "R503 ready");
    } else {
        ESP_LOGW(TAG, "Ready byte not received: %s", r503_err_to_name(err));
    }

    ESP_ERROR_CHECK(r503_handshake(&sensor));
    ESP_ERROR_CHECK(r503_check_sensor(&sensor));
    ESP_ERROR_CHECK(r503_verify_password(&sensor));

    r503_sys_params_t params = {0};
    ESP_ERROR_CHECK(r503_read_sys_params(&sensor, &params));

    ESP_LOGI(TAG, "Capacity       : %u", params.capacity);
    ESP_LOGI(TAG, "Security level : %u", params.security_level);
    ESP_LOGI(TAG, "Device address : 0x%08" PRIX32, params.device_address);

    r503_fw_version_t fw = {0};
    if (r503_get_fw_version(&sensor, &fw) == ESP_OK) {
        ESP_LOGI(TAG, "FW Version     : %s", fw.fw_version);
    }

    r503_alg_version_t alg = {0};
    if (r503_get_alg_version(&sensor, &alg) == ESP_OK) {
        ESP_LOGI(TAG, "Alg Version    : %s", alg.alg_version);
    }

    r503_product_info_t prod = {0};
    if (r503_read_product_info(&sensor, &prod) == ESP_OK) {
        ESP_LOGI(TAG, "Module Type    : %s", prod.module_type);
        ESP_LOGI(TAG, "Sensor Type    : %s", prod.sensor_type);
        ESP_LOGI(TAG, "Sensor Size    : %ux%u", prod.sensor_width, prod.sensor_height);
        ESP_LOGI(TAG, "Template Total : %u", prod.template_total);
    }

    uint16_t templates = 0;
    ESP_ERROR_CHECK(r503_template_count(&sensor, &templates));
    ESP_LOGI(TAG, "Stored templates: %u", templates);

    uint16_t next_free_id = 0;
    ESP_ERROR_CHECK(r503_find_next_free_id(&sensor, &next_free_id));
    ESP_LOGI(TAG, "Next free ID   : %u", next_free_id);

    uint16_t saved_id = 0;
    ESP_LOGI(TAG, "Place same finger for AUTO ENROLL...");
    err = r503_auto_enroll(&sensor,
                           next_free_id,
                           false,   /* allow_override */
                           true,    /* allow_duplicate */
                           true,    /* return_step_status */
                           true,    /* require_finger_leave */
                           &saved_id,
                           auto_enroll_cb,
                           NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Auto enroll failed: %s", r503_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Auto enroll success at ID %u", saved_id);

    r503_search_result_t result = {0};
    ESP_LOGI(TAG, "Place enrolled finger for AUTO IDENTIFY...");
    err = r503_auto_identify(&sensor,
                             3,      /* security level */
                             0,      /* start id */
                             200,    /* count */
                             true,   /* return step status */
                             1,      /* search error times */
                             &result,
                             auto_identify_cb,
                             NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Auto identify failed: %s", r503_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "AUTO MATCH! ID=%u Score=%u", result.match_id, result.match_score);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}