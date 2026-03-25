#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "r503.h"
#include "r503_defs.h"

static const char *TAG = "aura_demo";

static void run_led_step(r503_t *sensor,
                         const char *label,
                         r503_led_mode_t mode,
                         uint8_t speed,
                         r503_led_color_t color,
                         uint8_t times,
                         uint32_t delay_ms)
{
    esp_err_t err = r503_aura_led_config(sensor, mode, speed, color, times);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s failed: %s", label, r503_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "%s", label);
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
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

    ESP_LOGI(TAG, "Starting Aura LED demo...");

    while (1) {
        run_led_step(&sensor, "Blue always on",
                     R503_LED_ALWAYS_ON, 0x80, R503_LED_BLUE, 0, 1200);

        run_led_step(&sensor, "Cyan breathing",
                     R503_LED_BREATHING, 0x50, R503_LED_CYAN, 0, 2000);

        run_led_step(&sensor, "Red flashing x3",
                     R503_LED_FLASHING, 0x30, R503_LED_RED, 3, 2000);

        run_led_step(&sensor, "Green always on",
                     R503_LED_ALWAYS_ON, 0x80, R503_LED_GREEN, 1, 1200);

        run_led_step(&sensor, "White gradual on",
                     R503_LED_GRADUAL_ON, 0x40, R503_LED_WHITE, 0, 1500);

        run_led_step(&sensor, "LED off",
                     R503_LED_ALWAYS_OFF, 0x00, R503_LED_BLUE, 0, 1200);
    }
}