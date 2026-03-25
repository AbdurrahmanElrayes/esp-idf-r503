#include <stdio.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "r503.h"
#include "r503_defs.h"

static const char *TAG = "auto_example";

/* ================= CALLBACKS ================= */

static void auto_enroll_cb(const r503_auto_enroll_step_t *event, void *user_ctx)
{
    (void)user_ctx;
    ESP_LOGI("auto_enroll", "step=0x%02X id=%u",
             event->step, event->finger_id);
}

static void auto_identify_cb(const r503_auto_identify_step_t *event, void *user_ctx)
{
    (void)user_ctx;
    ESP_LOGI("auto_identify", "step=0x%02X id=%u score=%u",
             event->step, event->match_id, event->match_score);
}

/* ================= MAIN ================= */

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

    /* ===== READY / HANDSHAKE ===== */

    if (r503_wait_ready(&sensor, 1500) == ESP_OK) {
        ESP_LOGI(TAG, "Sensor ready");
    } else {
        ESP_LOGW(TAG, "Ready byte not received (normal on some modules)");
    }

    ESP_ERROR_CHECK(r503_handshake(&sensor));
    ESP_ERROR_CHECK(r503_check_sensor(&sensor));
    ESP_ERROR_CHECK(r503_verify_password(&sensor));

    ESP_LOGI(TAG, "Sensor initialized successfully");

    /* ===== INFO ===== */

    r503_sys_params_t params = {0};
    ESP_ERROR_CHECK(r503_read_sys_params(&sensor, &params));

    ESP_LOGI(TAG, "Capacity       : %u", params.capacity);
    ESP_LOGI(TAG, "Security level : %u", params.security_level);

    uint16_t templates = 0;
    ESP_ERROR_CHECK(r503_template_count(&sensor, &templates));
    ESP_LOGI(TAG, "Stored templates: %u", templates);

    /* ===== FIND NEXT ID ===== */

    uint16_t next_id = 0;
    ESP_ERROR_CHECK(r503_find_next_free_id(&sensor, &next_id));
    ESP_LOGI(TAG, "Next free ID: %u", next_id);

    /* ===== AUTO ENROLL ===== */

    ESP_LOGI(TAG, "=== AUTO ENROLL START ===");
    ESP_LOGI(TAG, "Place the same finger multiple times...");

    uint16_t saved_id = 0;

    esp_err_t err = r503_auto_enroll(&sensor,
                                    next_id,
                                    false,   /* allow_override */
                                    true,    /* allow_duplicate */
                                    true,    /* return step status */
                                    true,    /* require finger leave */
                                    &saved_id,
                                    auto_enroll_cb,
                                    NULL);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Auto enroll failed: %s", r503_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Enroll SUCCESS → ID=%u", saved_id);

    /* ===== AUTO IDENTIFY ===== */

    ESP_LOGI(TAG, "=== AUTO IDENTIFY START ===");
    ESP_LOGI(TAG, "Place your finger...");

    r503_search_result_t result = {0};

    err = r503_auto_identify(&sensor,
                            3,        /* security level */
                            0,        /* start id */
                            params.capacity,
                            true,     /* step status */
                            1,        /* retry */
                            &result,
                            auto_identify_cb,
                            NULL);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Auto identify failed: %s", r503_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "MATCH → ID=%u Score=%u",
             result.match_id,
             result.match_score);

    /* ===== LOOP ===== */

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}