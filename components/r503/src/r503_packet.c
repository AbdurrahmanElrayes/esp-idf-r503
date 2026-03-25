#include <string.h>
#include <stdint.h>

#include "esp_check.h"
#include "esp_log.h"

#include "r503_defs.h"
#include "r503_errors.h"
#include "r503_types.h"

static const char *TAG = "r503_packet";

static uint16_t be16_read(const uint8_t *p)
{
    return ((uint16_t)p[0] << 8) | p[1];
}

static uint32_t be32_read(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  |
           ((uint32_t)p[3]);
}

static void be16_write(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)((v >> 8) & 0xFF);
    p[1] = (uint8_t)(v & 0xFF);
}

static void be32_write(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)((v >> 24) & 0xFF);
    p[1] = (uint8_t)((v >> 16) & 0xFF);
    p[2] = (uint8_t)((v >> 8) & 0xFF);
    p[3] = (uint8_t)(v & 0xFF);
}

uint16_t r503_packet_checksum(uint8_t pid, uint16_t length, const uint8_t *payload, size_t payload_len)
{
    uint32_t sum = 0;
    sum += pid;
    sum += (length >> 8) & 0xFF;
    sum += length & 0xFF;

    for (size_t i = 0; i < payload_len; i++) {
        sum += payload[i];
    }

    return (uint16_t)(sum & 0xFFFF);
}

esp_err_t r503_packet_build(uint32_t address,
                            uint8_t pid,
                            const uint8_t *payload,
                            size_t payload_len,
                            uint8_t *out_buf,
                            size_t out_buf_size,
                            size_t *out_len)
{
    ESP_RETURN_ON_FALSE(out_buf != NULL, ESP_ERR_INVALID_ARG, TAG, "out_buf is NULL");
    ESP_RETURN_ON_FALSE(out_len != NULL, ESP_ERR_INVALID_ARG, TAG, "out_len is NULL");
    ESP_RETURN_ON_FALSE(payload_len <= 256, ESP_ERR_INVALID_ARG, TAG, "payload too large");

    const size_t total_len = 2 + 4 + 1 + 2 + payload_len + 2;
    ESP_RETURN_ON_FALSE(out_buf_size >= total_len, ESP_ERR_INVALID_SIZE, TAG, "buffer too small");

    const uint16_t length = (uint16_t)(payload_len + 2);
    const uint16_t checksum = r503_packet_checksum(pid, length, payload, payload_len);

    out_buf[0] = 0xEF;
    out_buf[1] = 0x01;
    be32_write(&out_buf[2], address);
    out_buf[6] = pid;
    be16_write(&out_buf[7], length);

    if (payload_len > 0 && payload != NULL) {
        memcpy(&out_buf[9], payload, payload_len);
    }

    be16_write(&out_buf[9 + payload_len], checksum);
    *out_len = total_len;

    return ESP_OK;
}

esp_err_t r503_packet_parse(const uint8_t *raw, size_t raw_len, r503_packet_t *packet)
{
    ESP_RETURN_ON_FALSE(raw != NULL, ESP_ERR_INVALID_ARG, TAG, "raw is NULL");
    ESP_RETURN_ON_FALSE(packet != NULL, ESP_ERR_INVALID_ARG, TAG, "packet is NULL");
    ESP_RETURN_ON_FALSE(raw_len >= 11, ESP_ERR_INVALID_SIZE, TAG, "raw too short");

    const uint16_t header = be16_read(&raw[0]);
    ESP_RETURN_ON_FALSE(header == R503_HEADER, ESP_ERR_R503_PACKET, TAG, "bad header");

    const uint8_t pid = raw[6];
    const uint16_t length = be16_read(&raw[7]);

    ESP_RETURN_ON_FALSE(length >= 2, ESP_ERR_R503_PACKET, TAG, "invalid length");

    const size_t expected_total = 2 + 4 + 1 + 2 + length;
    ESP_RETURN_ON_FALSE(raw_len == expected_total, ESP_ERR_R503_PACKET, TAG, "length mismatch");

    const size_t payload_len = length - 2;
    ESP_RETURN_ON_FALSE(payload_len <= sizeof(packet->payload), ESP_ERR_R503_PACKET, TAG, "payload too large");

    const uint16_t checksum_rx = be16_read(&raw[9 + payload_len]);
    const uint16_t checksum_calc = r503_packet_checksum(pid, length, &raw[9], payload_len);

    ESP_RETURN_ON_FALSE(checksum_rx == checksum_calc, ESP_ERR_R503_PACKET, TAG, "checksum mismatch");

    packet->header = header;
    packet->address = be32_read(&raw[2]);
    packet->pid = pid;
    packet->length = length;
    packet->checksum = checksum_rx;

    if (payload_len > 0) {
        memcpy(packet->payload, &raw[9], payload_len);
    }

    return ESP_OK;
}

uint16_t r503_packet_payload_len(const r503_packet_t *packet)
{
    if (packet == NULL || packet->length < 2) {
        return 0;
    }

    return (uint16_t)(packet->length - 2);
}