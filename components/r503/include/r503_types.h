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