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

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#include "r503.h"

#ifdef __cplusplus
extern "C" {
#endif

/* internal UART functions */
esp_err_t r503_uart_init(const r503_t *dev);
esp_err_t r503_uart_deinit(const r503_t *dev);
esp_err_t r503_uart_write(const r503_t *dev, const uint8_t *data, size_t len);
esp_err_t r503_uart_read_exact(const r503_t *dev, uint8_t *data, size_t len, uint32_t timeout_ms);
esp_err_t r503_uart_flush_input(const r503_t *dev);

/* internal packet functions */
uint16_t r503_packet_checksum(uint8_t pid, uint16_t length, const uint8_t *payload, size_t payload_len);
esp_err_t r503_packet_build(uint32_t address,
                            uint8_t pid,
                            const uint8_t *payload,
                            size_t payload_len,
                            uint8_t *out_buf,
                            size_t out_buf_size,
                            size_t *out_len);
esp_err_t r503_packet_parse(const uint8_t *raw, size_t raw_len, r503_packet_t *packet);
uint16_t r503_packet_payload_len(const r503_packet_t *packet);

/* shared internal helpers */
esp_err_t r503_map_confirm_code(uint8_t code);
esp_err_t r503_check_dev(const r503_t *dev);
esp_err_t r503_send_command(const r503_t *dev,
                            uint8_t command,
                            const uint8_t *params,
                            size_t params_len);
esp_err_t r503_read_packet(const r503_t *dev, r503_packet_t *packet, uint32_t timeout_ms);
esp_err_t r503_read_ack(const r503_t *dev, r503_packet_t *packet, uint8_t *confirm_code);
esp_err_t r503_read_ack_with_timeout(const r503_t *dev,
                                    r503_packet_t *packet,
                                    uint8_t *confirm_code,
                                    uint32_t timeout_ms);
uint16_t be16_from_payload(const uint8_t *p);
uint32_t be32_from_payload(const uint8_t *p);

esp_err_t r503_wait_for_finger_and_gen_char(r503_t *dev,
                                           uint8_t buffer_id,
                                           uint32_t capture_timeout_ms,
                                           uint32_t poll_delay_ms);
esp_err_t r503_wait_finger_removed_internal(r503_t *dev,
                                            uint32_t timeout_ms,
                                            uint32_t poll_delay_ms);

bool r503_index_bit_is_set(const uint8_t *bits, uint16_t local_id);
void r503_copy_ascii_field(char *dst, size_t dst_size, const uint8_t *src, size_t src_len);

#ifdef __cplusplus
}
#endif
