/*
 * Copyright 2026 Abdelrahman
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/uart.h"

typedef struct {
    uart_port_t uart_num;
    gpio_num_t tx_pin;
    gpio_num_t rx_pin;
    int baud_rate;
    uint32_t address;
    uint32_t password;
    uint32_t rx_timeout_ms;
    int uart_buffer_size;
} r503_config_t;

typedef struct {
    r503_config_t cfg;
    bool initialized;
} r503_t;

typedef struct {
    uint16_t header;
    uint32_t address;
    uint8_t pid;
    uint16_t length;          // payload + checksum(2)
    uint8_t payload[256];
    uint16_t checksum;
} r503_packet_t;

typedef struct {
    uint16_t status_register;
    uint16_t system_id;
    uint16_t capacity;
    uint16_t security_level;
    uint32_t device_address;
    uint16_t packet_size_code;
    uint16_t baud_multiplier;
} r503_sys_params_t;

typedef struct {
    uint16_t match_id;
    uint16_t match_score;
} r503_search_result_t;

typedef struct {
    uint8_t page_index;
    uint8_t bits[32];
} r503_index_table_t;

typedef struct {
    char fw_version[33];
} r503_fw_version_t;

typedef struct {
    char alg_version[33];
} r503_alg_version_t;

typedef struct {
    char module_type[17];
    char batch_number[5];
    char serial_number[9];
    uint8_t hw_ver_major;
    uint8_t hw_ver_minor;
    char sensor_type[9];
    uint16_t sensor_width;
    uint16_t sensor_height;
    uint16_t template_size;
    uint16_t template_total;
} r503_product_info_t;

typedef struct {
    uint16_t model_id;
    uint16_t capture_timeout_ms;
    uint16_t poll_delay_ms;
} r503_enroll_config_t;


typedef enum {
    R503_LED_BREATHING   = 0x01,
    R503_LED_FLASHING    = 0x02,
    R503_LED_ALWAYS_ON   = 0x03,
    R503_LED_ALWAYS_OFF  = 0x04,
    R503_LED_GRADUAL_ON  = 0x05,
    R503_LED_GRADUAL_OFF = 0x06,
} r503_led_mode_t;

typedef enum {
    R503_LED_RED    = 0x01,
    R503_LED_BLUE   = 0x02,
    R503_LED_PURPLE = 0x03,
    R503_LED_GREEN  = 0x04,
    R503_LED_YELLOW = 0x05,
    R503_LED_CYAN   = 0x06,
    R503_LED_WHITE  = 0x07,
} r503_led_color_t;

typedef struct {
    uint8_t confirmation_code;
    uint8_t step;
    uint16_t finger_id;
} r503_auto_enroll_step_t;

typedef struct {
    uint8_t confirmation_code;
    uint8_t step;
    uint16_t match_id;
    uint16_t match_score;
} r503_auto_identify_step_t;

typedef void (*r503_auto_enroll_callback_t)(const r503_auto_enroll_step_t *event, void *user_ctx);
typedef void (*r503_auto_identify_callback_t)(const r503_auto_identify_step_t *event, void *user_ctx);